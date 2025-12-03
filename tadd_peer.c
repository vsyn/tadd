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

#include "tadd_peer.h"

#include <limits.h>
#include <string.h>

static unsigned int sel_key(void *vkey, lbtree_index_t index) {
  return (((unsigned char *)vkey)[index / CHAR_BIT] >> (index % CHAR_BIT)) & 1;
}

/* does not handle duplicates */
void tadd_peer_tree_add(struct tadd *tadd, struct tadd_peer *peer) {
  struct tadd_peer **tree = &tadd->recv_routing;
  if (*tree == 0) {
    lbtree_init(&peer->tree);
    *tree = peer;
    return;
  }
  struct lbtree_test *match =
      (struct lbtree_test *)lbtree(&(*tree)->tree, tadd->sel_node, peer);
  lbtree_index_t index = 0;
  while (tadd->sel_node(peer, index) == tadd->sel_node(match, index)) {
    ++index;
  }
  peer->tree.index = index;
  lbtree_add((struct lbtree **)tree, tadd->sel_node, &peer->tree);
}

struct tadd_peer *tadd_peer_by_id(struct tadd *tadd, size_t id) {
  return (struct tadd_peer *)(((unsigned char *)tadd->send_routing) +
                              (id * tadd->peer_size));
}

void *tadd_peer_tree_lookup(struct tadd *tadd, unsigned char *key) {
  return lbtree((struct lbtree *)tadd->recv_routing, &sel_key, key);
}

void *tadd_peer_tree_walk(struct tadd *tadd,
                          void *(*action)(struct tadd_peer *peer,
                                          void *closure),
                          void *closure) {
  return lbtree_walk(&tadd->recv_routing->tree, tadd->sel_node,
                     (void *(*)(void *node, void *closure))action, closure);
}
