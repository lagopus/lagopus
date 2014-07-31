/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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


#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "confsys.h"
#include "cclient.h"
#include "lagopus/pbuf.h"

/* confsys client. */
struct cclient {
  /* Socket for the connection. */
  int sock_config;
  int sock_show;

  /* Packet buffer for sending packet. */
#define CCLIENT_PBUF_SIZE   4096
  struct pbuf *pbuf;

  /* Buffer for the message. */
  uint8_t *buf[1024];

  /* Message type. */
  enum confsys_msg_type type;
};

void
cclient_type_set(struct cclient *cclient, enum confsys_msg_type type) {
  cclient->type = type;
}

enum confsys_msg_type
cclient_type_get(struct cclient *cclient) {
  return cclient->type;
}

/* cclient allocation. */
struct cclient *
cclient_alloc(void) {
  struct cclient *cclient;

  cclient = (struct cclient *)calloc(1, sizeof(struct cclient));
  if (cclient == NULL) {
    return NULL;
  }

  cclient->pbuf = pbuf_alloc(CCLIENT_PBUF_SIZE);
  if (cclient->buf == NULL) {
    free(cclient);
    return NULL;
  }

  cclient->sock_config = -1;
  cclient->sock_show = -1;

  return cclient;
}

/* cclient free. */
void
cclient_free(struct cclient *cclient) {
  if (cclient->pbuf) {
    pbuf_free(cclient->pbuf);
  }
  free(cclient);
}

/* Open socket then connect to confsys server. */
static int
cclient_start(const char *path) {
  int ret;
  int sock;
  struct sockaddr_un sun;
  socklen_t len;

  /* UNIX domain socket. */
  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    return -1;
  }

  /* Prepare sockaddr_un. */
  memset(&sun, 0, sizeof(struct sockaddr_un));
  sun.sun_family = AF_UNIX;
  strncpy(sun.sun_path, path, strlen(path));
  len = SUN_LEN(&sun);

  /* Connect */
  ret = connect(sock, (struct sockaddr *)&sun, len);
  if (ret < 0) {
    close(sock);
    return -1;
  }

  /* Success. */
  return sock;
}

/* Stop confsys connection. */
void
cclient_stop(struct cclient *cclient) {
  /* Close socket. */
  if (cclient->sock_config >= 0) {
    close(cclient->sock_config);
    cclient->sock_config = -1;
  }
  if (cclient->sock_show >= 0) {
    close(cclient->sock_show);
    cclient->sock_show = -1;
  }

  /* Rest pbuf. */
  pbuf_reset(cclient->pbuf);
}

/* Send confsys message with arguments. */
static ssize_t
cclient_send_with_args(struct cclient *cclient, int sock,
                       enum confsys_msg_type type, int argc, char **argv) {
  int i;
  struct pbuf *pbuf;
  size_t length;
  ssize_t nbytes;

  /* Encode the message. */
  pbuf = cclient->pbuf;
  pbuf_reset(pbuf);

  /* Set header length. */
  length = CONFSYS_HEADER_LEN;

  /* Argument will be separated by space.  */
  for (i = 0; i < argc; i++) {
    length += strlen(argv[i]);
  }
  length += (size_t)(argc - 1);

  /* Encode message. */
  ENCODE_PUTW(type);
  ENCODE_PUTW(length);
  for (i = 0; i < argc; i++) {
    ENCODE_PUT(argv[i], strlen(argv[i]));
    if (i != argc - 1) {
      ENCODE_PUTC(' ');
    }
  }

  /* Write the message. */
  nbytes = write(sock, pbuf->data, (size_t)pbuf_readable_size(pbuf));

  return nbytes;
}

/* Display error message to cli. */
static void
cclient_error_display(enum config_result_t type) {
  switch (type) {
    case CONFIG_SUCCESS:
      break;
    case CONFIG_FAILURE:
      printf("\n%% Error occured.\n\n");
      break;
    case CONFIG_NO_CALLBACK:
      printf("\n%% Can't find callback function.\n\n");
      break;
    case CONFIG_ALREADY_EXIST:
      printf("\n%% Config node already exist.\n\n");
      break;
    case CONFIG_DOES_NOT_EXIST:
      printf("\n%% Can't find config node.\n\n");
      break;
    case CONFIG_SCHEMA_ERROR:
      printf("\n%% Config does not match to schema.\n\n");
      break;
  }
}

/* Receive confsys reply.  When the type is CONFSYS_MSG_TYPE_GET, it
 * is undefined length reply.  We will keep on reading until the
 * socket is closed. */
static ssize_t
cclient_recv(struct cclient *cclient, int sock, enum confsys_msg_type type) {
  ssize_t nbytes;
  struct pbuf *pbuf;
  struct confsys_header header;
  char *buf[BUFSIZ];
  size_t to_be_read;

  /* Set packet buffer. */
  pbuf = cclient->pbuf;
  pbuf_reset(pbuf);

  /* Read reply header. */
  nbytes = pbuf_read_size(pbuf, sock, CONFSYS_HEADER_LEN);
  if (nbytes <= 0 || nbytes != CONFSYS_HEADER_LEN) {
    cclient_stop(cclient);
    return -1;
  }

  /* Fetch type and length. */
  DECODE_GETW(header.type);
  DECODE_GETW(header.length);

  /* Display error message. */
  if (header.type != CONFIG_SUCCESS) {
    cclient_error_display(header.type);
  }

  /* Remaining data to be read. */
  to_be_read = header.length - CONFSYS_HEADER_LEN;

  /* Read until nothing to read. */
  if (type == CONFSYS_MSG_TYPE_GET) {
    while (1) {
      nbytes = read(sock, buf, BUFSIZ);
      if (nbytes <= 0) {
        return -1;
      }
      write(STDOUT_FILENO, buf, (size_t)nbytes);
    }
  } else {
    while (to_be_read) {
      nbytes = read(sock, buf, to_be_read);
      if (nbytes <= 0) {
        break;
      }
      write(STDOUT_FILENO, buf, (size_t)nbytes);
      to_be_read -= (size_t)nbytes;
    }
  }

  return nbytes;
}

/* Send confsys requet with arguments. */
int
cclient_request_with_args(struct cclient *cclient, int argc, char **argv) {
  int sock;
  enum confsys_msg_type type;
  ssize_t nbytes;

  /* Get type. */
  type = cclient->type;

  /* Start client connection. */
  if (type == CONFSYS_MSG_TYPE_GET) {
    cclient->sock_show = cclient_start(CONFSYS_SHOW_PATH);
    if (cclient->sock_show < 0) {
      printf("\n%% Can't open connection to lagopus\n\n");
      return -1;
    }
    sock = cclient->sock_show;
  } else {
    if (cclient->sock_config < 0) {
      cclient->sock_config = cclient_start(CONFSYS_CONFIG_PATH);
    }
    if (cclient->sock_config < 0) {
      printf("\n%% Can't open connection to lagopus\n\n");
      return -1;
    }
    sock = cclient->sock_config;
  }

  /* Send the message. */
  cclient_send_with_args(cclient, sock, type, argc, argv);

  /* Receive the message. */
  nbytes = cclient_recv(cclient, sock, type);

  /* Stop client connection. */
  if (type == CONFSYS_MSG_TYPE_GET) {
    cclient_stop(cclient);
  } else {
    if (nbytes < 0) {
      cclient_stop(cclient);
    } else {
      pbuf_reset(cclient->pbuf);
    }
  }

  return 0;
}
