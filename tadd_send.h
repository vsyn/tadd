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

#ifndef TADD_SEND_H
#define TADD_SEND_H

#include "tadd_types.h"

void tadd_send_cb(struct tadd_send *send, void (*cb)(void *closure),
                  void *closure);
void tadd_send_init(struct tadd_send *send, void (*cb)(void *closure),
                    void *closure);
int tadd_send_rst(struct tadd *tadd, size_t dst);
int tadd_send_unacked(struct tadd *tadd);
int tadd_send_peer_reset_unacked(struct tadd_peer *peer);
int tadd_send(struct tadd *tadd, size_t dst, void *data, size_t data_size);

#endif
