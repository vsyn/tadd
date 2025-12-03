/* Copyright 2022 Julian Ingram
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "tadd_socket.h"
#include "tadd_header.h"

#include <errno.h>
#include <fcntl.h>

#include <assert.h>

static const struct timeval TIMEOUT = {.tv_sec = 0, .tv_usec = 800000};

int tadd_socket_get_timeout(int sock, const struct timeval *timeout) {
  socklen_t len = sizeof(*timeout);
  return getsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)timeout, &len);
}

int tadd_socket_set_timeout(int sock, const struct timeval *timeout) {
  return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)timeout,
                    sizeof(*timeout));
}

int tadd_socket_send(int sock, struct sockaddr *sockaddr, socklen_t socklen,
                     struct tadd_data_linked *data) {
  ssize_t size = sendto(sock, data->data.buf, data->data.size, MSG_DONTWAIT,
                        sockaddr, socklen);
  if (size != (ssize_t)data->data.size) {
    if (size == -1) {
      if ((errno != EWOULDBLOCK) && (errno != EAGAIN)) {
        return -1;
      }
    }
    return 0; /* Either would block, or sent less than the full packet (sendto
    interrupted), either way requires resend */
  }
  return 0;
}

/* return values: error (-ve), got something - carry on (1),
 * and timed out/no-mem (0)
 */

static int _recv(struct tadd *tadd, int sock, struct sockaddr *sockaddr,
                 socklen_t *socklen, struct tadd_data_linked **dout,
                 int flags) {
  struct tadd_data_linked *data_linked = tadd_pool_alloc(&tadd->data_pool);
  if (data_linked == 0) {
    return 0;
  }

  ssize_t size = recvfrom(sock, data_linked->data.buf,
                          tadd->max_payload_size + TADD_HEADER_SIZE,
                          MSG_TRUNC | flags, sockaddr, socklen);
  if (size < 0) {
    tadd_pool_free(&tadd->data_pool, data_linked);
    if ((errno != EWOULDBLOCK) && (errno != EAGAIN)) {
      return -1;
    }
    /* timed out */
    return 0;
  }

  data_linked->data.size = size;
  *dout = data_linked;
  return 1;
}

int tadd_socket_recv(struct tadd *tadd, int sock, struct sockaddr *sockaddr,
                     socklen_t *socklen, struct tadd_data_linked **dout,
                     unsigned char blocking) {
  return _recv(tadd, sock, sockaddr, socklen, dout,
               (blocking != 0) ? 0 : MSG_DONTWAIT);
}

int tadd_socket_init(int domain, struct sockaddr *sockaddr, socklen_t socklen) {
  int sock = socket(domain, SOCK_DGRAM, 0);
  if (sock < 0) {
    return sock;
  }

  int rc = tadd_socket_set_timeout(sock, &TIMEOUT);
  if (rc < 0) {
    close(sock);
    return rc;
  }

  rc = bind(sock, sockaddr, socklen);
  if (rc < 0) {
    close(sock);
    return rc;
  }

  return sock;
}
