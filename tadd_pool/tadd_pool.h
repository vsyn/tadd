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

#ifndef TADD_POOL_H
#define TADD_POOL_H

#include "lblist.h"
#include "tadd_cirq.h"

#include <assert.h>

/* the length of the ptrs array must be one more than the number of pool buffers
 */
static inline void tadd_pool_init(struct tadd_cirq *pool, void **ptrs,
                                  unsigned char *buf, size_t num, size_t size) {
  pool->start = (unsigned char *)ptrs;
  pool->end = (unsigned char *)(ptrs + num);
  pool->head = pool->start;
  pool->tail = (unsigned char *)pool->end;
  pool->size = sizeof(void *);

  void **p = ptrs;
  while (p < (void **)pool->tail) {
    *p = buf;
    ++p;
    buf += size;
  }
}

static inline void *tadd_pool_alloc(struct tadd_cirq *pool) {
  void **v = tadd_cirq_rm(pool);
  return (v == 0) ? 0 : *v;
}

static inline void tadd_pool_free(struct tadd_cirq *pool, void *mem) {
  void **v = tadd_cirq_add(pool);
  assert(v != 0);
  *v = mem;
}

#endif