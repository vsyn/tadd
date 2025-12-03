# Tadd

Reliable, ordered, unlimited packet size transmission over UDP.

```
static char ADDR[] = "127.0.0.1";
static const int PORT = 3434;

static char PEER_ADDR[] = "127.0.0.1";
static const int PEER_PORT = 3435;

void recv_cb(struct tadd *tadd, struct lblist *data, void *closure) {
  char recv_buf[128];
  tadd_recv(tadd, recv_buf, 0, sizeof(recv_buf), data);
  printf("%s\n", recv_buf);
}

void send_example(void) { 
    struct tadd_ipv4_peer peer;
    struct tadd_ipv4 tadd;

    /* initialise, error checking omitted for brevity */
    tadd_ipv4_init(&tadd, ADDR, PORT, &peers[0], 1);
    tadd_ipv4_peer_add(&tadd, &peer, PEER_ADDR, PEER_PORT, 0, 0, recv_cb, 0);

    /* send "hello" to peer 0 */
    tadd_send(&tadd, 0, "hello", 6);
    while (1) {
        /* accept the ack, recv_cb also called from idle on reciept */
        tadd_idle_blocking(&tadd);
    }
}

```