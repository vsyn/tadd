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

#include "tadd_ipv4.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>

static int tadd_ipv4_parse_addr(struct sockaddr_in *sockaddr, char *ip_str,
                                unsigned short int port) {
  struct in_addr ip;
  int rc = inet_aton(ip_str, &ip);
  /* returns zero on failure */
  if (rc == 0) {
    return -1;
  }
  struct sockaddr_in _sockaddr = {
      .sin_family = AF_INET, .sin_addr = ip, .sin_port = htons(port)};
  *sockaddr = _sockaddr;
  return 0;
}

static int _recv(struct tadd *tadd, struct tadd_data_linked **data,
                 unsigned char **addr, unsigned char blocking) {
  struct tadd_ipv4 *ti4 = (struct tadd_ipv4 *)tadd;
  *addr = (unsigned char *)&ti4->addr;
  socklen_t addr_size = sizeof(ti4->addr);
  int rc = tadd_socket_recv(tadd, ti4->sock, (struct sockaddr *)&ti4->addr,
                            &addr_size, data, blocking);
  if (rc < 0) {
    return rc;
  }
  if (addr_size != sizeof(ti4->addr)) {
    tadd_pool_free(&tadd->data_pool, *data);
    return -1;
  }
  return rc;
}

static int tadd_ipv4_recv(struct tadd *tadd, struct tadd_data_linked **data,
                          unsigned char **addr) {
  return _recv(tadd, data, addr, 0);
}

static int tadd_ipv4_recv_blocking(struct tadd *tadd,
                                   struct tadd_data_linked **data,
                                   unsigned char **addr) {
  return _recv(tadd, data, addr, 1);
}

static int tadd_ipv4_send(struct tadd *tadd, struct tadd_peer *peer,
                          struct tadd_data_linked *data) {
  struct tadd_ipv4 *ti4 = (struct tadd_ipv4 *)tadd;
  struct tadd_ipv4_peer *pi4 = (struct tadd_ipv4_peer *)peer;
  return tadd_socket_send(ti4->sock, (struct sockaddr *)&pi4->addr,
                          sizeof(pi4->addr), data);
}

size_t tadd_ipv4_peer_sizeof(void) { return sizeof(struct tadd_ipv4_peer); }

size_t tadd_ipv4_sizeof(void) { return sizeof(struct tadd_ipv4); }

int tadd_ipv4_peer_add(struct tadd_ipv4 *tadd4, struct tadd_ipv4_peer *peer4,
                       char *ip_str, unsigned short int port,
                       void (*recv_cb)(struct tadd *tadd, struct lblist *data,
                                       void *closure),
                       void *recv_closure, void (*send_cb)(void *closure),
                       void *send_closure) {
  int rc = tadd_ipv4_parse_addr(&peer4->addr, ip_str, port);
  if (rc != 0) {
    return rc;
  }
  tadd_peer_add(&tadd4->tadd, &peer4->peer, recv_cb, recv_closure, send_cb,
                send_closure);
  return 0;
}

static unsigned int tadd_ipv4_sel_key(void *vkey, lbtree_index_t index) {
  return (((unsigned char *)vkey)[index / CHAR_BIT] >> (index % CHAR_BIT)) & 1;
}

static unsigned int tadd_ipv4_sel_node(void *vkey, lbtree_index_t index) {
  struct tadd_ipv4_peer *peer = vkey;
  return tadd_ipv4_sel_key(&peer->addr, index);
}

static int tadd_ipv4_verify_addr(struct tadd_peer *peer, unsigned char *addr) {
  struct tadd_ipv4_peer *peer4 = (struct tadd_ipv4_peer *)peer;
  size_t i;
  for (i = 0; i < sizeof(peer4->addr); ++i) {
    if (addr[i] != ((unsigned char *)&peer4->addr)[i]) {
      return 1;
    }
  }
  return 0;
}

int tadd_ipv4_init(struct tadd_ipv4 *tadd4, char *ip_str,
                   unsigned short int port, struct tadd_ipv4_peer *peers,
                   size_t peers_size) {
  struct sockaddr_in sockaddr;
  int rc = tadd_ipv4_parse_addr(&sockaddr, ip_str, port);
  if (rc != 0) {
    return rc;
  }

  int sock =
      tadd_socket_init(AF_INET, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
  if (sock < 0) {
    return sock;
  }

  tadd4->sock = sock;

  struct tadd *tadd = &tadd4->tadd;
  tadd->send = &tadd_ipv4_send;
  tadd->recv = &tadd_ipv4_recv;
  tadd->recv_blocking = &tadd_ipv4_recv_blocking;
  tadd->sel_node = &tadd_ipv4_sel_node;
  tadd->verify_addr = &tadd_ipv4_verify_addr;
  tadd->max_payload_size = TADD_IPV4_MAX_PAYLOAD_SIZE;
  tadd->peer_size = sizeof(struct tadd_ipv4_peer);
  tadd_pool_init(&tadd->data_pool, tadd4->data_pool_ptrs, tadd4->data_pool_buf,
                 TADD_IPV4_DATA_POOL_SIZE,
                 (sizeof(struct tadd_data_linked) + TADD_IPV4_MAX_PACKET_SIZE));
  tadd_init(tadd, (struct tadd_peer *)peers, peers_size);
  return 0;
}

void tadd_ipv4_destroy(struct tadd_ipv4 *tadd4) { close(tadd4->sock); }
