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

#ifndef TADD_IPV4_H
#define TADD_IPV4_H

#include "tadd_header.h"
#include "tadd_socket.h"

#define TADD_IPV4_MAX_PAYLOAD_SIZE (1024 * 16)
#define TADD_IPV4_DATA_POOL_SIZE (1024)

#define TADD_IPV4_MAX_PACKET_SIZE                                              \
  (TADD_IPV4_MAX_PAYLOAD_SIZE + TADD_HEADER_SIZE)

struct tadd_ipv4 {
  struct tadd tadd;
  struct sockaddr_in addr; /* used as a scratch pad for incoming messages */
  int sock;
  void *data_pool_ptrs[TADD_IPV4_DATA_POOL_SIZE + 1];
  unsigned char data_pool_buf[(sizeof(struct tadd_data_linked) +
                               TADD_IPV4_MAX_PACKET_SIZE) *
                              TADD_IPV4_DATA_POOL_SIZE];
};

struct tadd_ipv4_peer {
  struct tadd_peer peer;
  struct sockaddr_in addr;
};

size_t tadd_ipv4_peer_sizeof(void);
size_t tadd_ipv4_sizeof(void);
int tadd_ipv4_peer_add(struct tadd_ipv4 *tadd4, struct tadd_ipv4_peer *peer4,
                       char *ip_str, unsigned short int port,
                       void (*recv_cb)(struct tadd *tadd, struct lblist *data,
                                       void *closure),
                       void *recv_closure, void (*send_cb)(void *closure),
                       void *send_closure);
int tadd_ipv4_init(struct tadd_ipv4 *tadd4, char *ip_str,
                   unsigned short int port, struct tadd_ipv4_peer *peers,
                   size_t peers_size);
void tadd_ipv4_destroy(struct tadd_ipv4 *tadd4);

#endif
