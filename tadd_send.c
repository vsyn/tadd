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

#include "tadd_send.h"

#include "tadd_header.h"
#include "tadd_recv.h"

#include "tadd_peer.h"

#include <string.h>

/* Should be 0 for production, look at how this is used before setting */
#ifndef TADD_SEND_TEST_FAIL_PROB
#define TADD_SEND_TEST_FAIL_PROB (0)
#endif

void tadd_send_cb(struct tadd_send *send, void (*cb)(void *closure),
                  void *closure) {
  send->cb = cb;
  send->closure = closure;
}

void tadd_send_init(struct tadd_send *send, void (*cb)(void *closure),
                    void *closure) {
  send->unacked = 0;
  send->id = 0;
  tadd_send_cb(send, cb, closure);
}

void *tadd_send_peer_clear_action(void *node, void *closure) {
  struct tadd *tadd = (struct tadd *)closure;
  tadd_pool_free(&tadd->data_pool, node);
  return 0;
}

int tadd_send_rst_peer(struct tadd *tadd, struct tadd_peer *peer) {
  lbtree_uint_walk((struct lbtree_uint *)peer->send.unacked,
                   &tadd_send_peer_clear_action, tadd);
  peer->send.unacked = 0;

  struct tadd_header header = {.id = peer->send.id,
                               .type = TADD_HEADER_TYPE_RST};

  struct tadd_data_linked *data;
  unsigned char buf[sizeof(*data) + TADD_HEADER_SIZE];
  data = (struct tadd_data_linked *)buf;
  data->data.size = TADD_HEADER_SIZE;

  tadd_header_serialise(data->data.buf, &header);

  peer->send.id = tadd_header_id_inc(peer->send.id);

  return tadd->send(tadd, peer, data);
}

int tadd_send_rst(struct tadd *tadd, size_t dst) {
  return tadd_send_rst_peer(tadd, tadd_peer_by_id(tadd, dst));
}

static int tadd_send_dgram(struct tadd *tadd, struct tadd_peer *peer,
                           unsigned char type, unsigned char *data,
                           size_t data_size) {
  struct tadd_data_linked *dgram_data = tadd_pool_alloc(&tadd->data_pool);
  if (dgram_data == 0) {
    return -1;
  }
  dgram_data->data.size = data_size + TADD_HEADER_SIZE;

  struct tadd_header header = {.id = peer->send.id, .type = type};

  tadd_header_serialise(dgram_data->data.buf, &header);
  memcpy(dgram_data->data.buf + TADD_HEADER_SIZE, data, data_size);

  header = tadd_header_parse(dgram_data->data.buf);

  /* save in unacked until ack */
  dgram_data->links.tree.key = header.id;
  void *repl = lbtree_uint_add((struct lbtree_uint **)&peer->send.unacked,
                               &dgram_data->links.tree);
  if (repl != 0) {
    tadd_pool_free(&tadd->data_pool, repl);
  }

  peer->send.id = tadd_header_id_inc(peer->send.id);

#if TADD_TX_TEST_FAIL_PROB != 0
  if (rand() < TADD_TX_TEST_FAIL_PROB) {
    return 0;
  }
#endif

  return tadd->send(tadd, peer, dgram_data);
}

struct tadd_send_unacked_closure {
  struct tadd *tadd;
  struct tadd_peer *peer;
};

static void *tadd_send_unacked_peer_action(void *node, void *closure) {
  struct tadd_send_unacked_closure *t =
      (struct tadd_send_unacked_closure *)closure;
  return (t->tadd->send(t->tadd, t->peer, (struct tadd_data_linked *)node) == 0)
             ? 0
             : closure;
}

int tadd_send_unacked(struct tadd *tadd) {
  struct tadd_send_unacked_closure send_closure = {.tadd = tadd};
  for (size_t i = 0, s = tadd->peers_size; i < s; ++i) {
    struct tadd_peer *peer = tadd_peer_by_id(tadd, i);
    send_closure.peer = peer;
    void *c = lbtree_uint_walk((struct lbtree_uint *)peer->send.unacked,
                               &tadd_send_unacked_peer_action, &send_closure);
    if (c != 0) {
      return -1;
    }
  }
  return 0;
}

static int tadd_send_peer(struct tadd *tadd, struct tadd_peer *peer,
                          unsigned char type, void *data, size_t data_size) {
  size_t to_send = data_size;
  size_t max_payload_size = tadd->max_payload_size;
  unsigned char *buf = data;
  while (to_send >= max_payload_size) {
    int rc = tadd_send_dgram(tadd, peer, type, buf, max_payload_size);
    if (rc != 0) {
      return rc;
    }
    to_send -= max_payload_size;
    buf += max_payload_size;
  }
  return tadd_send_dgram(tadd, peer, type, buf, to_send);
}

int tadd_send(struct tadd *tadd, size_t dst, void *data, size_t data_size) {
  return tadd_send_peer(tadd, tadd_peer_by_id(tadd, dst), TADD_HEADER_TYPE_DATA,
                        data, data_size);
}
