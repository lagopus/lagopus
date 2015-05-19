/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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

enum confsys_msg_type {
  CONFSYS_MSG_TYPE_GET            = 0,
  CONFSYS_MSG_TYPE_SET            = 1,
  CONFSYS_MSG_TYPE_SET_VALIDATE   = 2,
  CONFSYS_MSG_TYPE_UNSET          = 3,
  CONFSYS_MSG_TYPE_UNSET_VALIDATE = 4,
  CONFSYS_MSG_TYPE_EXEC           = 5,
  CONFSYS_MSG_TYPE_LOCK           = 6,
  CONFSYS_MSG_TYPE_UNLOCK         = 7
};

#define CONFSYS_IPC_PATH "/tmp/.lagopus"

struct header {
  uint32_t type;
  uint32_t length;
};

int
main(int argc, char **argv) {
  int i;
  int ret;
  int sock;
  struct sockaddr_un sun;
  socklen_t len;
  struct header header;
  char buf[BUFSIZ];
  char *str = "capable-switch";

  /* Create socket. */
  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    fprintf(stderr, "can't open socket\n");
  }

  /* Prepare sockaddr_un. */
  memset(&sun, 0, sizeof(struct sockaddr_un));
  sun.sun_family = AF_UNIX;
  strncpy(sun.sun_path, CONFSYS_IPC_PATH, strlen(CONFSYS_IPC_PATH));
  len = SUN_LEN(&sun);

  /* Connect */
  ret = connect(sock, (struct sockaddr *)&sun, len);
  if (ret < 0) {
    fprintf(stderr, "connect error\n");
    close(sock);
    return -1;
  }

  /* Prepare packet. */
  header.type = CONFSYS_MSG_TYPE_GET;
  header.length = 4 + strlen(str);

  *((uint32_t *)buf) = htons(header.type);
  *((uint32_t *)(buf + 2)) = htons(header.length);

  memcpy(buf + 4, str, strlen(str));

  /* Write. */
  ret = write(sock, buf, header.length);
  printf("write return %d\n", ret);

  /* Read header. */
  ret = read(sock, buf, 4);
  if (ret <= 0) {
    perror("header read error\n");
    return -1;
  }

  /* Debug return value. */
  header.type = *((uint32_t *)buf);
  header.type = ntohs(header.type);
  header.length = *((uint32_t *)(buf + 2));
  header.length = ntohs(header.length);

  /* Read */
  while (1) {
    ret = read(sock, buf, BUFSIZ);
    if (ret <= 0) {
      break;
    }
    write(1, buf, ret);
  }

  return 0;
}
