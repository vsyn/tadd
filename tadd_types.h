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

#ifndef TADD_TYPES_H
#define TADD_TYPES_H

#include "lblist.h"
#include "lbtree_uint.h"
#include "tadd_pool.h"

struct tadd_data {
  size_t size;
  unsigned char buf[];
};

union tadd_data_links {
  struct lbtree_uint tree;
  struct lblist list;
};

struct tadd_data_linked {
  union tadd_data_links links;
  struct tadd_data data;
};

struct tadd_data_out {
  struct lblist list;
  struct tadd_data_linked *end;
};

struct tadd;

struct tadd_recv {
  /* queue containing ordered dgrams */
  struct lblist ordered;
  /* tree containing out of order dgrams */
  struct lbtree_uint *unordered;
  /* packet id, used to request a specific retransmission */
  unsigned long int id;
  /* callback called for each incoming message */
  void (*cb)(struct tadd *tadd, struct lblist *data, void *closure);
  void *closure;
};

struct tadd_send {
  /* stores unacked dgrams */
  struct tadd_data_linked *unacked;
  /* packet id, used to request a specific retransmission */
  unsigned long int id;
  /* callback called when last unacked message acked.
   * if a further message is sent without waiting for the acks from the first,
   * this callback will only trigger once */
  void (*cb)(void *closure);
  void *closure;
};

struct tadd_peer {
  struct lbtree tree;
  struct tadd_send send;
  struct tadd_recv recv;
};

struct tadd {
  /* pointer to array of peers in order */
  struct tadd_peer *send_routing;
  /* gets a peer struct from a socket addr */
  struct tadd_peer *recv_routing;
  /* pool for the transmit and recieve packetised internal data */
  struct tadd_cirq data_pool;
  /* should be implemented to send a packet in the usr data buf to the address
   * in addr */
  int (*send)(struct tadd *tadd, struct tadd_peer *peer,
              struct tadd_data_linked *data);

  int (*recv)(struct tadd *tadd, struct tadd_data_linked **data,
              unsigned char **addr);
  /* recv_blocking is expected to block for a short amount of time (any unacked
   * transmissions will be resent after this period) and if nothing is received,
   * return a usr data structure with size 0 and buffer 0 */
  int (*recv_blocking)(struct tadd *tadd, struct tadd_data_linked **data,
                       unsigned char **addr);
  unsigned int (*sel_node)(void *vkey, lbtree_index_t index);
  int (*verify_addr)(struct tadd_peer *peer, unsigned char *addr);
  size_t max_payload_size;
  size_t peer_size;
  size_t peers_size;
};

#endif
