#include "../lbtree/tests/everand/everand.h"
#include "../tadd_ipv4.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static char LH[] = "127.0.0.1";
static const int PORTS[] = {3434, 3435, 3436, 3457};

#define PEER_COUNT (sizeof(PORTS) / sizeof(PORTS[0]))

#define MSG_SIZE_MAX (100000)
static const unsigned long int MSG_COUNT = 1000;

struct state {
  unsigned char send_buf[MSG_SIZE_MAX];
  unsigned char recv_buf[MSG_SIZE_MAX];
  ssize_t send_size;
  unsigned long int sent;
  unsigned int done;
  unsigned long int peer_id;
  size_t peer_done_count;
};

/* messages have id as their first byte, then random size and content */
ssize_t gen_random_message(unsigned char *send_buf, unsigned int id) {
  if (id > 255) {
    return -1;
  }
  size_t size;
  size = everand(MSG_SIZE_MAX - 1) + 1;
  // printf("size: %zu\n", size);

  send_buf[0] = id;
  everand_arr(&(send_buf[1]), size - 1);
  return size;
}

void recv_cb(struct tadd *tadd, struct lblist *data, void *closure) {
  struct state *state = closure;
  size_t recv_size = tadd_recv(tadd, state->recv_buf, 0, MSG_SIZE_MAX, data);
  if (recv_size < 0) {
    // printf("recv err: %s\n", strerror(errno));
    return;
  }
  if (recv_size == 0) {
    // printf("peer done %lu\n", state->peer_done_count);
    ++state->peer_done_count;
    return;
  }
  // printf("recvd for: %u\n", state->recv_buf[0]);
  if (state->recv_buf[0] == state->peer_id) {
    // printf("match: %lu, %ld, %ld\n", state->sent, recv_size,
    // state->send_size);
    assert(recv_size == state->send_size);
    assert(memcmp(state->recv_buf, state->send_buf, state->send_size) == 0);
    unsigned long int peer_queue;
    state->send_size = -1;
    if (state->sent < MSG_COUNT) {
      do { // this should go in recv_cb
        peer_queue = rand() % PEER_COUNT;
      } while (peer_queue == state->peer_id);
      state->send_size = gen_random_message(state->send_buf, state->peer_id);
      // printf("transmitting for: %u, to: %lu \n", state->send_buf[0],
      // peer_queue);
      int rc = tadd_send(tadd, peer_queue, state->send_buf, state->send_size);
      if (rc != 0) {
        printf("send err: %s\n", strerror(errno));
      }
      assert(rc == 0);
      ++state->sent;
    } else if (state->done == 0) {
      /* send zero length to indicate done */
      size_t i;
      for (i = 0; i < PEER_COUNT; ++i) {
        if (i == state->peer_id) {
          continue;
        }
        int rc = tadd_send(tadd, i, state->send_buf, 0);
        if (rc != 0) {
          printf("send err: %s\n", strerror(errno));
        }
        assert(rc == 0);
      }
      state->done = 1;
      printf("done %lu\n", state->peer_done_count);
    }
    return;
  }

  /* send to another random peer */
  unsigned long int peer_queue;
  do {
    peer_queue = rand() % PEER_COUNT;
  } while (peer_queue == state->peer_id);
  // printf("retransmitting to %lu\n", peer_queue);
  assert(tadd_send(tadd, peer_queue, state->recv_buf, recv_size) == 0);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("takes 1 arg\n");
    return -1;
  }

  struct state state = {.sent = 1, .done = 0, .peer_done_count = 1};

  struct tadd_ipv4 tadd = {0};
  struct tadd_ipv4_peer peers[PEER_COUNT] = {0};
  state.peer_id = strtol(argv[1], 0, 0);

  srand(state.peer_id + 7);

  // printf("peer id: %lu\n", state.peer_id);

  assert(tadd_ipv4_init(&tadd, LH, PORTS[state.peer_id], peers, PEER_COUNT) ==
         0);
  unsigned long int i = 0;
  while (i < PEER_COUNT) {
    assert(tadd_ipv4_peer_add(&tadd, &peers[i], LH, PORTS[i], recv_cb, &state,
                              0, 0) == 0);
    ++i;
  }

  unsigned long int peer_queue;
  do {
    peer_queue = rand() % PEER_COUNT;
  } while (peer_queue == state.peer_id);
  state.send_size = gen_random_message(state.send_buf, state.peer_id);
  // printf("transmitting for: %u, to: %lu \n", state.send_buf[0], peer_queue);
  int rc = tadd_send(&tadd.tadd, peer_queue, state.send_buf, state.send_size);
  if (rc != 0) {
    printf("send err: %s\n", strerror(errno));
  }
  assert(rc == 0);

  while ((state.done == 0) || (state.peer_done_count < PEER_COUNT)) {
    tadd_idle_blocking(&tadd.tadd);
  }

  tadd_ipv4_destroy(&tadd);

  printf("cleaned up %lu\n", state.peer_id);

  return 0;
}
