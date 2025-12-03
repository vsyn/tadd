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

#ifndef TADD_RECV_H
#define TADD_RECV_H

#include "tadd_types.h"

size_t tadd_recv_src_id(struct tadd *tadd, struct lblist *ordered);
void tadd_recv_cb(struct tadd_recv *recv,
                  void (*cb)(struct tadd *tadd, struct lblist *data,
                             void *closure),
                  void *closure);
void tadd_recv_init(struct tadd_recv *recv, unsigned long int order_id,
                    void (*cb)(struct tadd *tadd, struct lblist *data,
                               void *closure),
                    void *closure);
void tadd_recv_rst(struct tadd *tadd, unsigned long int src,
                   unsigned long int order_id);

size_t tadd_recv_buf_get(struct lblist *data, unsigned char **buf);
void tadd_recv_buf_rm(struct tadd *tadd, struct lblist *data);

int tadd_recv_accept_all(struct tadd *tadd);
int tadd_recv_accept_all_blocking(struct tadd *tadd);
int tadd_idle(struct tadd *tadd);
int tadd_idle_blocking(struct tadd *tadd);

size_t tadd_recv(struct tadd *tadd, unsigned char *buf, size_t offset,
                 size_t size, struct lblist *data);

#endif