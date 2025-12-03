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

#include "tadd.h"

#include "tadd_header.h"
#include "tadd_peer.h"

static void tadd_peer_init(struct tadd_peer *peer,
                           void (*recv_cb)(struct tadd *tadd,
                                           struct lblist *data, void *closure),
                           void *recv_closure, void (*send_cb)(void *closure),
                           void *send_closure) {
  tadd_send_init(&peer->send, send_cb, send_closure);
  tadd_recv_init(&peer->recv, 0, recv_cb, recv_closure);
}

void tadd_peer_add(struct tadd *tadd, struct tadd_peer *peer,
                   void (*recv_cb)(struct tadd *tadd, struct lblist *data,
                                   void *closure),
                   void *recv_closure, void (*send_cb)(void *closure),
                   void *send_closure) {
  tadd_peer_init(peer, recv_cb, recv_closure, send_cb, send_closure);
  tadd_peer_tree_add(tadd, peer);
}

void tadd_init(struct tadd *tadd, struct tadd_peer *peers, size_t peers_size) {
  tadd->send_routing = peers;
  tadd->peers_size = peers_size;
  tadd->recv_routing = 0;
}
