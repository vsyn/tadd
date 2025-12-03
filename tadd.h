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

#ifndef TADD_H
#define TADD_H

/* Tadd
 * a communications framework built to overcome 3 shortcomings of UDP,
 * it:
 *  * has an unlimited packet size
 *  * orders incoming packets
 *  * tries retansmission in case of failure while still being single threaded
 * assumes a UDP like protocol that ensures packet integrity.
 */

#include "tadd_recv.h"
#include "tadd_send.h"

void tadd_peer_add(struct tadd *tadd, struct tadd_peer *peer,
                   void (*recv_cb)(struct tadd *tadd, struct lblist *data,
                                   void *closure),
                   void *recv_closure, void (*send_cb)(void *closure),
                   void *send_closure);
void tadd_init(struct tadd *tadd, struct tadd_peer *peers, size_t peers_size);

#endif
