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

/* Forward declaration. */
struct event;
struct event_manager;

/* Event manager allocation and free. */
struct event_manager *event_manager_alloc(void);
void event_manager_free(struct event_manager *em);

/* Event register functions. */
struct event *event_register_event(struct event_manager *em,
                                   void (*func)(struct event *), void *arg);
struct event *event_register_read(struct event_manager *em, int fd,
                                  void (*func)(struct event *), void *arg);
struct event *event_register_write(struct event_manager *em, int fd,
                                   void (*func)(struct event *), void *arg);
struct event *event_register_timer(struct event_manager *em,
                                   void (*func)(struct event *), void *arg,
                                   long timer);

/* Event cancel function. */
void event_cancel(struct event **event);

/* Event getter function. */
void *event_get_arg(struct event *event);
int event_get_sock(struct event *event);
struct event_manager *event_get_manager(struct event *event);

/* Event fetch and call. */
struct event *event_fetch(struct event_manager *em);
void event_call(struct event *event);

/* Stop event fetch loop. */
void event_manager_stop(struct event_manager *em);
