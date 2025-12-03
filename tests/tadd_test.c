#include "../tadd_ipv4.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <stdio.h>

static char LH[] = "127.0.0.1";
static const int PORTS[] = {3434, 3435};

struct tadd_ipv4_peer peers[2];
struct tadd_ipv4 tadds[2];

void tadd_init_pair(void) {
  assert(tadd_ipv4_init(&tadds[0], LH, PORTS[0], &peers[0], 1) == 0);
  assert(tadd_ipv4_peer_add(&tadds[0], &peers[0], LH, PORTS[1], 0, 0, 0, 0) ==
         0);

  assert(tadd_ipv4_init(&tadds[1], LH, PORTS[1], &peers[1], 1) == 0);
  assert(tadd_ipv4_peer_add(&tadds[1], &peers[1], LH, PORTS[0], 0, 0, 0, 0) ==
         0);
}

void tadd_destroy_pair(void) {
  tadd_ipv4_destroy(&tadds[0]);
  tadd_ipv4_destroy(&tadds[1]);
}

void test_send_cb(struct tadd *tadd, struct lblist *data, void *closure) {
  unsigned char *buf;
  size_t size = tadd_recv_buf_get(data, &buf);
  assert(size == 5);
  assert(strcmp((char *)buf, "test") == 0);
  tadd_recv_buf_rm(tadd, data);
  int *flag = closure;
  *flag = 1;
}

void test_send(unsigned int dir) {
  int flag = 0;
  struct tadd *src = &tadds[dir & 1].tadd;
  struct tadd_recv *dst_peer_recv =
      &tadds[(dir & 1) ^ 1].tadd.send_routing[0].recv;
  dst_peer_recv->cb = &test_send_cb;
  dst_peer_recv->closure = &flag;
  assert(tadd_send(src, 0, "test", 5) == 0);
  tadd_idle_blocking(&tadds[(dir & 1) ^ 1].tadd);
  assert(flag == 1);
  dst_peer_recv->cb = 0;
  dst_peer_recv->closure = 0;
}

void test_normal(void) {
  tadd_init_pair();

  test_send(0);

  tadd_destroy_pair();
}

void test_out_of_order_cb(struct tadd *tadd, struct lblist *data,
                          void *closure) {
  unsigned int *state = closure;
  unsigned char *buf;
  size_t size = tadd_recv_buf_get(data, &buf);
  switch (*state) {
  case 0:
    assert(size == 4);
    assert(strcmp((char *)buf, "tes") == 0);
    break;
  case 1:
    assert(size == 5);
    assert(strcmp((char *)buf, "test") == 0);
    break;
  default:
    assert(0);
    break;
  }
  ++*state;
  tadd_recv_buf_rm(tadd, data);
}

void test_out_of_order(unsigned long int start) {
  tadd_init_pair();

  unsigned long int dst = 0;
  struct tadd_peer *src_peer = &peers[0].peer;
  struct tadd_peer *dst_peer = &peers[1].peer;
  src_peer->send.id = (start + 1) & TADD_ID_MASK;
  assert(tadd_send(&tadds[0].tadd, dst, "test", 5) == 0);
  src_peer->send.id = start;
  assert(tadd_send(&tadds[0].tadd, dst, "tes", 4) == 0);

  dst_peer->recv.id = start;

  unsigned int state = 0;
  dst_peer->recv.cb = &test_out_of_order_cb;
  dst_peer->recv.closure = &state;
  tadd_idle_blocking(&tadds[1].tadd);

  tadd_destroy_pair();
}

void test_reset(void) {
  tadd_init_pair();

  unsigned long int dst = 0;
  int rc;
  // do {
  rc = tadd_send(&tadds[0].tadd, dst, "test", 5);
  //} while (rc >= 0);
  (void)rc;
  tadds[1].tadd.send_routing[0].recv.id = 1234;

  tadd_send_rst(&tadds[0].tadd, dst);

  tadd_recv_accept_all_blocking(&tadds[1].tadd);
  tadd_recv_accept_all_blocking(&tadds[1].tadd);

  // printf("%lu\n", tadds[1].tadd.send_routing[0]->recv.id);

  test_send(0);

  tadd_destroy_pair();
}

struct test_large_closure {
  unsigned char *send_buf;
  size_t count;
};

#define TEST_LARGE_BUF_SIZE (81234)

void test_large_cb(struct tadd *tadd, struct lblist *data, void *closure) {
  struct test_large_closure *tlc = closure;
  unsigned char recv_buf[TEST_LARGE_BUF_SIZE];
  size_t size = tadd_recv(tadd, recv_buf, 0, TEST_LARGE_BUF_SIZE, data);
  assert(size == TEST_LARGE_BUF_SIZE);

  assert(memcmp(recv_buf, tlc->send_buf, TEST_LARGE_BUF_SIZE) == 0);
  ++tlc->count;
}

void test_large(void) {
  tadd_init_pair();

  unsigned char send_buf[TEST_LARGE_BUF_SIZE];

  size_t i = 0;
  while (i < sizeof(send_buf)) {
    ((unsigned char *)send_buf)[i] = i & 0xff;
    ++i;
  }

  struct test_large_closure tlc = {.send_buf = send_buf, .count = 0};

  struct tadd_recv *dst_peer_recv = &tadds[1].tadd.send_routing[0].recv;
  dst_peer_recv->cb = &test_large_cb;
  dst_peer_recv->closure = &tlc;

  assert(tadd_cirq_add(&tadds[0].tadd.data_pool) == 0);
  assert(tadd_cirq_add(&tadds[1].tadd.data_pool) == 0);

  i = 0;
  while (i < 10000) {
    assert(tadd_send(&tadds[0].tadd, 0, send_buf, sizeof(send_buf)) == 0);
    assert(tadd_idle_blocking(&tadds[1].tadd) == 0);
    assert(tadd_idle_blocking(&tadds[0].tadd) == 0);
    ++i;
  }
  assert(tlc.count == 10000);

  assert(tadd_cirq_add(&tadds[0].tadd.data_pool) == 0);
  assert(tadd_cirq_add(&tadds[1].tadd.data_pool) == 0);

  tadd_destroy_pair();
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  test_normal();
  test_out_of_order(0);
  test_out_of_order(3333);
  test_out_of_order(0x3ffffffful);
  test_reset();
  test_large();
  return 0;
}
