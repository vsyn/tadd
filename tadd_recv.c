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

#include "tadd_recv.h"

#include "tadd_header.h"
#include "tadd_send.h"

#include "tadd_peer.h"

#include <string.h>

size_t tadd_recv_src_id(struct tadd *tadd, struct lblist *ordered) {
  return (((unsigned char *)ordered) - ((unsigned char *)tadd->send_routing)) /
         tadd->peer_size;
}

void tadd_recv_cb(struct tadd_recv *recv,
                  void (*cb)(struct tadd *tadd, struct lblist *data,
                             void *closure),
                  void *closure) {
  recv->cb = cb;
  recv->closure = closure;
}

static void init(struct tadd_recv *recv, unsigned long int order_id) {
  lblist_init(&recv->ordered);
  recv->unordered = 0;
  recv->id = order_id;
}

void tadd_recv_init(struct tadd_recv *recv, unsigned long int order_id,
                    void (*cb)(struct tadd *tadd, struct lblist *data,
                               void *closure),
                    void *closure) {
  init(recv, order_id);
  tadd_recv_cb(recv, cb, closure);
}

void *tadd_send_peer_clear_action(void *node, void *closure);

void tadd_recv_rst_peer(struct tadd *tadd, struct tadd_peer *peer,
                        unsigned long int order_id) {
  lbtree_uint_walk((struct lbtree_uint *)peer->recv.unordered,
                   &tadd_send_peer_clear_action, tadd);

  struct lblist *ordered = &peer->recv.ordered;
  while (1) {
    struct tadd_data_linked *dl = lblist_rm_tail(ordered);
    if (&dl->links.list == ordered) {
      break;
    }
    tadd_pool_free(&tadd->data_pool, dl);
  }

  init(&peer->recv, order_id);
}

void tadd_recv_rst(struct tadd *tadd, unsigned long int src,
                   unsigned long int order_id) {
  tadd_recv_rst_peer(tadd, tadd_peer_by_id(tadd, src), order_id);
}

static int tadd_recv_send_response(struct tadd *tadd, struct tadd_peer *peer,
                                   unsigned char type, unsigned long int id) {
  struct tadd_header header = {.id = id, .type = type};
  struct tadd_data_linked *data;
  unsigned char buf[sizeof(*data) + TADD_HEADER_SIZE];
  data = (struct tadd_data_linked *)buf;
  data->data.size = TADD_HEADER_SIZE;
  tadd_header_serialise(data->data.buf, &header);
  return tadd->send(tadd, peer, data);
}

size_t tadd_recv_buf_get(struct lblist *data, unsigned char **buf) {
  if (lblist_is_empty(data) != 0) {
    *buf = 0;
    return 0;
  }
  struct tadd_data_linked *ldata = (struct tadd_data_linked *)data->head;
  *buf = ldata->data.buf + TADD_HEADER_SIZE;
  return ldata->data.size - TADD_HEADER_SIZE;
}

void tadd_recv_buf_rm(struct tadd *tadd, struct lblist *data) {
  if (lblist_is_empty(data) != 0) {
    return;
  }
  struct tadd_data_linked *ldata = lblist_rm_head(data);
  tadd_pool_free(&tadd->data_pool, ldata);
}

static void tadd_recv_msg_ordered(struct tadd *tadd, struct tadd_recv *recv,
                                  struct tadd_data_linked *ldata,
                                  unsigned long int id) {
  size_t data_payload_size = ldata->data.size - TADD_HEADER_SIZE;
  lblist_ins_tail(&recv->ordered, &ldata->links.list);
  recv->id = tadd_header_id_inc(id);
  if (data_payload_size != tadd->max_payload_size) {
    if (recv->cb != 0) {
      recv->cb(tadd, &recv->ordered, recv->closure);
    }
  }
}

static void tadd_recv_handle_data(struct tadd *tadd, struct tadd_peer *src_peer,
                                  struct tadd_data_linked *ldata) {
  struct tadd_recv *recv = &src_peer->recv;
  unsigned long int id = tadd_header_id_parse(ldata->data.buf);
  (void)tadd_recv_send_response(tadd, src_peer, TADD_HEADER_TYPE_ACK, id);
  if (id != recv->id) {
    /* out of order */
    if (tadd_header_id_ooo(recv->id, id) != 0) {
      /* too badly out of order - ack sent above */
      tadd_pool_free(&tadd->data_pool, ldata);
      return;
    }

    ldata->links.tree.key = id;
    void *repl = lbtree_uint_add(&recv->unordered, &ldata->links.tree);
    if (repl != 0) {
      tadd_pool_free(&tadd->data_pool, repl);
    }
    return;
  }
  /* in order */
  tadd_recv_msg_ordered(tadd, recv, ldata, id);

  while (1) {
    ldata = lbtree_uint(recv->unordered, recv->id);
    if (ldata == 0) {
      break;
    }
    /* stored unordered dgram becomes ordered */
    lbtree_uint_rm(&recv->unordered, &ldata->links.tree);
    tadd_recv_msg_ordered(tadd, recv, ldata, recv->id);
  }
}

static void tadd_recv_handle_ack(struct tadd *tadd, struct tadd_peer *src_peer,
                                 struct tadd_data_linked *ldata) {
  unsigned long int id = tadd_header_id_parse(ldata->data.buf);
  struct tadd_send *send = &src_peer->send;
  tadd_pool_free(&tadd->data_pool, ldata);
  struct tadd_data_linked *acked = lbtree_uint(&send->unacked->links.tree, id);
  if (acked != 0) {
    lbtree_uint_rm((struct lbtree_uint **)&send->unacked, &acked->links.tree);
    tadd_pool_free(&tadd->data_pool, acked);
    if ((send->unacked == 0) && (send->cb != 0)) {
      send->cb(send->closure);
    }
  }
}

static void tadd_recv_handle_rst(struct tadd *tadd, struct tadd_peer *src_peer,
                                 struct tadd_data_linked *ldata) {
  unsigned long int id = tadd_header_id_parse(ldata->data.buf);
  tadd_pool_free(&tadd->data_pool, ldata);
  id = tadd_header_id_inc(id);
  tadd_recv_rst_peer(tadd, src_peer, id);
}

static int _tadd_recv_accept_all(struct tadd *tadd,
                                 int (*recv)(struct tadd *tadd,
                                             struct tadd_data_linked **data,
                                             unsigned char **addr)) {
  unsigned char *addr;
  struct tadd_data_linked *data;
  int rc = recv(tadd, &data, &addr);
  if (rc < 0) {
    return rc;
  }
  if (rc == 0) {
    /* timed out or no-mem */
    return 0;
  }
  if ((data->data.size > (tadd->max_payload_size + TADD_HEADER_SIZE)) ||
      (data->data.size < TADD_HEADER_SIZE)) {
    /* too big or too small */
    tadd_pool_free(&tadd->data_pool, data);
    return 1;
  }

  /* look up source */
  struct tadd_peer *src_peer = tadd_peer_tree_lookup(tadd, addr);

  if ((src_peer == 0) || (tadd->verify_addr(src_peer, addr))) {
    /* unknown source */
    tadd_pool_free(&tadd->data_pool, data);
    return 1;
  }

  unsigned char type = tadd_header_type_parse(data->data.buf);

  switch (type) {
  case TADD_HEADER_TYPE_DATA:
    tadd_recv_handle_data(tadd, src_peer, data);
    break;
  case TADD_HEADER_TYPE_ACK:
    tadd_recv_handle_ack(tadd, src_peer, data);
    break;
  case TADD_HEADER_TYPE_RST:
    tadd_recv_handle_rst(tadd, src_peer, data);
    break;
  }
  return 1;
}

int tadd_recv_accept_all(struct tadd *tadd) {
  return _tadd_recv_accept_all(tadd, tadd->recv);
}

int tadd_recv_accept_all_blocking(struct tadd *tadd) {
  return _tadd_recv_accept_all(tadd, tadd->recv_blocking);
}

int tadd_idle(struct tadd *tadd) {
  int found;
  do {
    found = tadd_recv_accept_all(tadd);
    if (found < 0) {
      return found;
    }
  } while (found != 0);
  return tadd_send_unacked(tadd);
}

int tadd_idle_blocking(struct tadd *tadd) {
  int found = tadd_recv_accept_all_blocking(tadd);
  if (found < 0) {
    return found;
  }
  return (found == 0) ? tadd_send_unacked(tadd) : tadd_idle(tadd);
}

static ssize_t tadd_recv_size_get(struct lblist *list, size_t max_size,
                                  size_t max_payload_size) {
  ssize_t count = 0;
  struct tadd_data_linked *el = list->head;
  while (el->data.size == max_size) {
    count += max_payload_size;
    el = el->links.list.tail;
  }
  return count + el->data.size - TADD_HEADER_SIZE;
}

size_t tadd_recv(struct tadd *tadd, unsigned char *buf, size_t offset,
                 size_t size, struct lblist *data) {
  const size_t max_payload_size = tadd->max_payload_size;
  const size_t max_size = max_payload_size + TADD_HEADER_SIZE;
  if (size == 0) {
    return tadd_recv_size_get(data, max_size, max_payload_size);
  }
  size_t next_write_count;
  struct tadd_data_linked *el;
  size_t write_count = 0;
  size_t offs = (offset % max_payload_size) + TADD_HEADER_SIZE;
  do {
    el = data->head;
    size_t dgram_payload_size = el->data.size - offs;
    next_write_count = write_count + dgram_payload_size;
    if (next_write_count > size) {
      size_t copy_size = size - write_count;
      next_write_count = write_count + copy_size;
      memcpy(buf + write_count, el->data.buf + offs, copy_size);
      break;
    }

    memcpy(buf + write_count, el->data.buf + offs, dgram_payload_size);
    lblist_rm(&el->links.list);
    tadd_pool_free(&tadd->data_pool, el);
    offs = TADD_HEADER_SIZE;
    write_count = next_write_count;
  } while (el->data.size == max_size);

  return next_write_count;
}