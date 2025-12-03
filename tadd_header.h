#ifndef TADD_HEADER_H
#define TADD_HEADER_H

#include <stdint.h>

#define TADD_HEADER_SIZE (4)

#define TADD_HEADER_TYPE_DATA (0x00)
#define TADD_HEADER_TYPE_ACK (0x01)
#define TADD_HEADER_TYPE_RST (0x02)

static const unsigned long int TADD_ORDER_MAX_LOOKAHEAD = 64;
static const unsigned long int TADD_ID_MASK = 0x3fffffff;

struct tadd_header {
  unsigned long int id;
  unsigned char type;
};

static inline unsigned long int tadd_header_id_parse(unsigned char *b) {
  uint8_t *b8 = (uint8_t *)b;
  return (((unsigned long int)b8[0] << 24) | ((unsigned long int)b8[1] << 16) |
          ((unsigned long int)b8[2] << 8) | b8[3]) &
         TADD_ID_MASK;
}

static inline unsigned char tadd_header_type_parse(unsigned char *b) {
  uint8_t *b8 = (uint8_t *)b;
  return b8[0] >> 6;
}

static inline struct tadd_header tadd_header_parse(unsigned char *buf) {
  struct tadd_header header = {.id = tadd_header_id_parse(buf),
                               .type = tadd_header_type_parse(buf)};
  return header;
}

static inline void tadd_header_serialise(unsigned char *b,
                                         struct tadd_header *header) {
  uint8_t *b8 = (uint8_t *)b;
  unsigned long int id = header->id;
  b8[0] = (header->type << 6) | (id >> 24);
  b8[1] = (id >> 16) & 0xff;
  b8[2] = (id >> 8) & 0xff;
  b8[3] = id & 0xff;
}

static inline unsigned long int tadd_header_id_inc(unsigned long int id) {
  return ++id & TADD_ID_MASK;
}

static inline int tadd_header_id_ooo(unsigned long int expected,
                                     unsigned long int received) {
  return (((received - expected) & TADD_ID_MASK) > TADD_ORDER_MAX_LOOKAHEAD)
             ? 1
             : 0;
}

#endif
