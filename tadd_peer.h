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

#ifndef TADD_PEER_H
#define TADD_PEER_H

#include "lbtree.h"
#include "tadd.h"

void tadd_peer_tree_add(struct tadd *tadd, struct tadd_peer *lbtt);
struct tadd_peer *tadd_peer_by_id(struct tadd *tadd, size_t id);
void *tadd_peer_tree_lookup(struct tadd *tadd, unsigned char *key);
void *tadd_peer_tree_walk(struct tadd *tadd,
                          void *(*action)(struct tadd_peer *peer,
                                          void *closure),
                          void *closure);

#endif
