/*
 * Copyright 2014-2016 Nippon Telegraph and Telephone Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "lagopus_apis.h"

#define __USE_BSD /* for timercmp/timeradd/timersub */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>

#include "event.h"
#include "timeutil.h"

/* Define 'struct event_list' for linked list of struct event.  */
TAILQ_HEAD(event_list, event);

/* Event manager. */
struct event_manager {
  /* Timer */
  int index;

  /* Timer slot. */
#define EVENT_TIMER_SLOT           4
  struct event_list timer[EVENT_TIMER_SLOT];

  /* Event lists.  */
  struct event_list ready;
  struct event_list read;
  struct event_list write;
  struct event_list event;
  struct event_list unuse;

  fd_set readfd;
  fd_set writefd;
  fd_set exceptfd;
  int max_fd;
  uint32_t alloc;

  /* Event manager lock */
  lagopus_mutex_t lock;

  /* event fetch runnable flag */
  bool run;
};

/* Thread types.  */
enum event_type {
  EVENT_TYPE_READY   = 0,
  EVENT_TYPE_EVENT   = 1,
  EVENT_TYPE_READ    = 2,
  EVENT_TYPE_WRITE   = 3,
  EVENT_TYPE_TIMER   = 4,
  EVENT_TYPE_RUNNING = 5,
  EVENT_TYPE_UNUSE   = 6
};

/* Event. */
struct event {
  /* Linked list. */
  TAILQ_ENTRY(event) entry;

  /* Pointer to the struct event_manager. */
  struct event_manager *manager;

  /* Event type. */
  enum event_type type;

  /* Event function. */
  void (*func)(struct event *);

  /* Event argument. */
  void *arg;

  /* File descriptor. */
  int fd;

  /* Timer. */
  int index;
  struct timeval sands;
};

/* Prototypes. */
struct event *event_pop_head(struct event_list *list);
struct event *event_get(struct event_manager *em, enum event_type type,
                        void (*func)(struct event *), void *arg);
void event_cancel_internal(struct event *event);
void event_free(struct event *event);
static void event_ready_add(struct event_manager *manager,
                            struct event *event);
static struct timeval *event_timer_wait(struct event_manager *em,
                                        struct timeval *timer_val,
                                        struct timeval *timer_now);
static int event_process_fd(struct event_manager *em, struct event_list *list,
                            fd_set *fdset, fd_set *mfdset);

static void
event_manager_lock(struct event_manager *em) {
  lagopus_msg_debug(10, "lock %p\n", &em->lock);
  lagopus_mutex_lock(&em->lock);
}

static void
event_manager_unlock(struct event_manager *em) {
  lagopus_msg_debug(10, "lock %p\n", &em->lock);
  lagopus_mutex_unlock(&em->lock);
}

/* Getter for arg in event. */
void *
event_get_arg(struct event *event) {
  return event->arg;
}

/* Getter for socket in event. */
int
event_get_sock(struct event *event) {
  return event->fd;
}

/* Getter for event_manager in event. */
struct event_manager *
event_get_manager(struct event *event) {
  return event->manager;
}

/* Pop first node from the list. */
struct event *
event_pop_head(struct event_list *list) {
  struct event *event = TAILQ_FIRST(list);

  if (event) {
    TAILQ_REMOVE(list, event, entry);
  }

  return event;
}

/* Get event.  Reuse allocated memory from unused list. */
static struct event *
event_get_nolock(struct event_manager *em, enum event_type type,
                 void (*func)(struct event *), void *arg) {
  struct event *event;

  if (TAILQ_FIRST(&em->unuse) != NULL) {
    event = event_pop_head(&em->unuse);
  } else {
    event = (struct event *)calloc(1, sizeof (struct event));
    if (event == NULL) {
      return NULL;
    }
    em->alloc++;
  }
  event->manager = em;
  event->type = type;
  event->func = func;
  event->arg = arg;

  return event;
}

struct event *
event_get(struct event_manager *em, enum event_type type,
          void (*func)(struct event *), void *arg) {
  struct event *event;

  event_manager_lock(em);
  event = event_get_nolock(em, type, func, arg);
  event_manager_unlock(em);

  return event;
}

/* Cancel event for internal use. */
void
event_cancel_internal(struct event *event) {
  struct event_manager *manager = event->manager;

  switch (event->type) {
    case EVENT_TYPE_READY:
      TAILQ_REMOVE(&manager->ready, event, entry);
      break;
    case EVENT_TYPE_EVENT:
      TAILQ_REMOVE(&manager->event, event, entry);
      break;
    case EVENT_TYPE_READ:
      FD_CLR(event->fd, &manager->readfd);
      TAILQ_REMOVE(&manager->read, event, entry);
      break;
    case EVENT_TYPE_WRITE:
      FD_CLR(event->fd, &manager->writefd);
      TAILQ_REMOVE(&manager->write, event, entry);
      break;
    case EVENT_TYPE_TIMER:
      TAILQ_REMOVE(&manager->timer[event->index], event, entry);
      break;
    case EVENT_TYPE_RUNNING:
    case EVENT_TYPE_UNUSE:
      /* Ignore running and unuse event. */
      return;
      break;
    default:
      break;
  }
  event->type = EVENT_TYPE_UNUSE;
  TAILQ_INSERT_TAIL(&manager->unuse, event, entry);
}

/* Cancel event with clearing event pointer of the caller. */
void
event_cancel(struct event **event) {
  struct event_manager *em;

  if (event == NULL || *event == NULL) {
    return;
  }

  em = (*event)->manager;
  event_manager_lock(em);
  event_cancel_internal(*event);
  *event = NULL;
  event_manager_unlock(em);
}

/* Free event. */
void
event_free(struct event *event) {
  free(event);
}

/* Register event. */
struct event *
event_register_event(struct event_manager *em,
                     void (*func)(struct event *), void *arg) {
  struct event *event;

  lagopus_msg_debug(10, "in\n");
  event_manager_lock(em);
  event = event_get_nolock(em, EVENT_TYPE_EVENT, func, arg);
  if (event == NULL) {
    event_manager_unlock(em);
    return NULL;
  }

  TAILQ_INSERT_TAIL(&em->event, event, entry);
  event_manager_unlock(em);

  return event;
}

/* Register read socket. */
struct event *
event_register_read(struct event_manager *em, int fd,
                    void (*func)(struct event *), void *arg) {
  struct event *event;

  lagopus_msg_debug(10, "in fd=%d\n", fd);
  if (fd < 0) {
    return NULL;
  }

  event_manager_lock(em);
  event = event_get_nolock(em, EVENT_TYPE_READ, func, arg);
  if (event == NULL) {
    event_manager_unlock(em);
    return NULL;
  }

  if (em->max_fd < fd) {
    em->max_fd = fd;
  }
  FD_SET(fd, &em->readfd);
  event->fd = fd;
  TAILQ_INSERT_TAIL(&em->read, event, entry);
  event_manager_unlock(em);

  return event;
}

/* Register write socket.  */
struct event *
event_register_write(struct event_manager *em, int fd,
                     void (*func)(struct event *), void *arg) {
  struct event *event = NULL;

  lagopus_msg_debug(10, "in fd=%d\n", fd);
  event_manager_lock(em);
  if (fd < 0) {
    goto done;
  }

  event = event_get_nolock(em, EVENT_TYPE_WRITE, func, arg);
  if (event == NULL) {
    goto done;
  }

  if (em->max_fd < fd) {
    em->max_fd = fd;
  }
  FD_SET(fd, &em->writefd);
  event->fd = fd;
  TAILQ_INSERT_TAIL(&em->write, event, entry);

done:
  event_manager_unlock(em);
  return event;
}

/* Register timer. */
struct event *
event_register_timer(struct event_manager *em, void (*func)(struct event *),
                     void *arg, long timer) {
  struct event *event;
  struct event *et;

  lagopus_msg_debug(10, "in\n");
  event_manager_lock(em);
  event = event_get_nolock(em, EVENT_TYPE_TIMER, func, arg);
  if (event == NULL) {
    goto done;
  }

  /* Set time and index. */
  gettimeofday(&event->sands, NULL);
  event->sands.tv_sec += timer;
  event->index = em->index;

  /* Sort by timeval. */
  for (et = TAILQ_LAST(&em->timer[em->index], event_list); et;
       et = TAILQ_PREV(et, event_list, entry))
    if (timercmp (&event->sands, &et->sands, -) >= 0) {
      break;
    }
  if (et != NULL) {
    TAILQ_INSERT_AFTER(&em->timer[em->index], et, event, entry);
  } else {
    TAILQ_INSERT_TAIL(&em->timer[em->index], event, entry);
  }

  /* Increment timer slot index.  */
  em->index++;
  em->index %= EVENT_TIMER_SLOT;

done:
  event_manager_unlock(em);

  return event;
}

/* Move to ready queue. */
/* Assume event manager locked. */
static void
event_ready_add(struct event_manager *em, struct event *event) {
  event->type = EVENT_TYPE_READY;
  TAILQ_INSERT_TAIL(&em->ready, event, entry);
}

/* Pick up the smallest timer.  */
/* Assume event manager locked. */
static struct timeval *
event_timer_wait(struct event_manager *em, struct timeval *timer_val,
                 struct timeval *timer_now) {
  struct timeval timer_min;
  struct timeval timer_default = {0, 1 * 100 * 1000}; /* default 100 msec wait */
  struct timeval *timer_wait;
  struct event *event;
  int i;

  timer_wait = NULL;

  for (i = 0; i < EVENT_TIMER_SLOT; i++)
    if ((event = TAILQ_FIRST(&em->timer[i])) != NULL) {
      if (! timer_wait) {
        timer_wait = &event->sands;
      } else if (timercmp(&event->sands, timer_wait, -) < 0) {
        timer_wait = &event->sands;
      }
    }

  if (timer_wait) {
    timer_min = *timer_wait;
    timersub(&timer_min, timer_now, &timer_min);

    if (timer_min.tv_sec < 0) {
      timer_min.tv_sec = 0;
      timer_min.tv_usec = 10;
    }

    if (timercmp(&timer_min, &timer_default, -) > 0) {
      *timer_val = timer_default;
    } else {
      *timer_val = timer_min;
    }

    return timer_val;
  }
  *timer_val = timer_default;
  return timer_val;
}

/* Process ready socket. */
/* Assume event manager locked. */
static int
event_process_fd(struct event_manager *em, struct event_list *list,
                 fd_set *fdset, fd_set *mfdset) {
  struct event *event;
  struct event *next;
  int ready = 0;

  for (event = TAILQ_FIRST(list); event; event = next) {
    next = TAILQ_NEXT(event, entry);

    if (FD_ISSET(event->fd, fdset)) {
      FD_CLR(event->fd, mfdset);
      TAILQ_REMOVE(list, event, entry);
      event_ready_add(em, event);
      ready++;
    }
  }
  return ready;
}

/* Return event which can be executed. */
struct event *
event_fetch(struct event_manager *em) {
  int i;
  int num;
  struct event *event;
  struct event *next;
  fd_set readfd;
  fd_set writefd;
  fd_set exceptfd;
  struct timeval timer_now;
  struct timeval timer_val;
  struct timeval *timer_wait;
  struct timeval timer_nowait;

  /* Zero wait timer. */
  timer_nowait.tv_sec = 0;
  timer_nowait.tv_usec = 0;

  lagopus_msg_debug(10, "event_fetch in\n");
  while (em->run) {
    event_manager_lock(em);
    /* Ready event return. */
    if ((event = event_pop_head(&em->ready)) != NULL) {
      event->type = EVENT_TYPE_RUNNING;
      event_manager_unlock(em);
      return event;
    }

    /* Move event to next ready queue to be executed.  */
    while ((event = event_pop_head(&em->event)) != NULL) {
      event_ready_add(em, event);
    }

    /* Get current time.  */
    gettimeofday(&timer_now, NULL);

    /* Expired timer is put into the queue. */
    for (i = 0; i < EVENT_TIMER_SLOT; i++)
      for (event = TAILQ_FIRST(&em->timer[i]); event; event = next) {
        next = TAILQ_NEXT(event, entry);

        if (timercmp(&timer_now, &event->sands, -) >= 0) {
          TAILQ_REMOVE(&em->timer[i], event, entry);
          event_ready_add(em, event);
        } else {
          break;
        }
      }

    /* Structure copy.  */
    readfd = em->readfd;
    writefd = em->writefd;
    exceptfd = em->exceptfd;

    /* When queue exists, use zero wait timer.  */
    if (TAILQ_FIRST(&em->ready) != NULL) {
      timer_wait = &timer_nowait;
    } else {
      timer_wait = event_timer_wait(em, &timer_val, &timer_now);
    }

    event_manager_unlock(em);
    lagopus_msg_debug(10, "timer_wait in %p\n", timer_wait);
    num = select(em->max_fd + 1, &readfd, &writefd, &exceptfd, timer_wait);
    lagopus_msg_debug(10, "timer_wait out %p\n", timer_wait);

    /* Error handling.  We ignore EINTR.  */
    if (num < 0) {
      if (errno == EINTR) {
        continue;
      }
      return NULL;
    }

    /* Socket readable/writable check. */
    if (num > 0) {
      event_manager_lock(em);
      event_process_fd(em, &em->read, &readfd, &em->readfd);
      event_process_fd(em, &em->write, &writefd, &em->writefd);
      event_manager_unlock(em);
    }
  }

  return NULL;
}

/* Execute the event. */
void
event_call(struct event *event) {
  (*event->func)(event);
  event_manager_lock(event->manager);
  event->type = EVENT_TYPE_UNUSE;
  TAILQ_INSERT_TAIL(&event->manager->unuse, event, entry);
  event_manager_unlock(event->manager);
}

/* Allocate event manager. */
struct event_manager *
event_manager_alloc(void) {
  int i;
  struct event_manager *em;
  lagopus_result_t ret;

  em = (struct event_manager *)calloc(1, sizeof(struct event_manager));
  for (i = 0; i < EVENT_TIMER_SLOT; i++) {
    TAILQ_INIT(&em->timer[i]);
  }
  TAILQ_INIT(&em->ready);
  TAILQ_INIT(&em->read);
  TAILQ_INIT(&em->write);
  TAILQ_INIT(&em->event);
  TAILQ_INIT(&em->unuse);

  ret = lagopus_mutex_create(&em->lock);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    event_manager_free(em);
    return NULL;
  }

  em->run = true;

  return em;
}

/* Free event manager. */
void
event_manager_free(struct event_manager *em) {
  int i;
  struct event *event;

  for (i = 0; i < EVENT_TIMER_SLOT; i++)
    while ((event = event_pop_head(&em->timer[i])) != NULL) {
      event_free(event);
    }
  while ((event = event_pop_head(&em->ready)) != NULL) {
    event_free(event);
  }
  while ((event = event_pop_head(&em->read)) != NULL) {
    event_free(event);
  }
  while ((event = event_pop_head(&em->write)) != NULL) {
    event_free(event);
  }
  while ((event = event_pop_head(&em->event)) != NULL) {
    event_free(event);
  }
  while ((event = event_pop_head(&em->unuse)) != NULL) {
    event_free(event);
  }

  if (em->lock != NULL) {
    lagopus_mutex_destroy(&em->lock);
  }

  free(em);
}

void
event_manager_stop(struct event_manager *em) {
  em->run = false;
}
