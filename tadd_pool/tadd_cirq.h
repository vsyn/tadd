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

#ifndef TADD_CIRQ_H
#define TADD_CIRQ_H

#include <stdlib.h>

struct tadd_cirq {
  unsigned char *head;
  unsigned char *tail;
  unsigned char *start;
  unsigned char *end;
  size_t size;
};

/* the length of the buffer must be (num + 1) * size */
static inline void tadd_cirq_init(struct tadd_cirq *cirq, unsigned char *buf,
                                  size_t num, size_t size) {
  cirq->head = buf;
  cirq->tail = buf;
  cirq->start = buf;
  /* Unchecked multiply, will not overflow if buffer fits in memory */
  cirq->end = buf + (num * size);
  cirq->size = size;
}

static inline void *tadd_cirq_add(struct tadd_cirq *cirq) {
  unsigned char *tail = cirq->tail;
  unsigned char *next_tail =
      (tail == cirq->end) ? cirq->start : tail + cirq->size;
  if (next_tail == cirq->head) {
    return 0;
  }
  cirq->tail = next_tail;
  return tail;
}

static inline void *tadd_cirq_rm(struct tadd_cirq *cirq) {
  if (cirq->head == cirq->tail) {
    return 0;
  }
  unsigned char *head = cirq->head;
  cirq->head = (head == cirq->end) ? cirq->start : head + cirq->size;
  return head;
}

#endif