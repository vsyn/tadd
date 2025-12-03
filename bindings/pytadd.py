from ctypes import *
tadd = CDLL("./libtadd.so")

c_send_cb_type = CFUNCTYPE(c_void_p, c_void_p)
c_recv_cb_type = CFUNCTYPE(c_void_p, c_void_p, c_void_p, c_void_p)

tadd.tadd_ipv4_init.argtypes = [c_void_p, c_char_p, c_ushort, c_void_p,
    c_size_t]
tadd.tadd_ipv4_init.restype = c_int

tadd.tadd_ipv4_peer_add.argtypes = [c_void_p, c_void_p, c_char_p, c_ushort,
    c_recv_cb_type, c_void_p, c_send_cb_type, c_void_p]
tadd.tadd_ipv4_peer_add.restype = c_int

tadd.tadd_recv_src_id.argtypes = [c_void_p, c_void_p]
tadd.tadd_recv_src_id.restype = c_size_t

tadd.tadd_recv.argtypes = [c_void_p, c_void_p, c_size_t, c_size_t, c_void_p]
tadd.tadd_recv.restype = c_size_t

tadd.tadd_idle.argtypes = [c_void_p]
tadd.tadd_idle.restype = c_int

tadd.tadd_idle_blocking.argtypes = [c_void_p]
tadd.tadd_idle_blocking.restype = c_int

tadd.tadd_send_rst.argtypes = [c_void_p, c_size_t]
tadd.tadd_send_rst.restype = c_int

tadd.tadd_send.argtypes = [c_void_p, c_size_t, c_void_p,  c_size_t]
tadd.tadd_send.restype = c_int

class tadd_ipv4():
    def __init__(self, addr, port, peers, callback):
        tadd_size = tadd.tadd_ipv4_sizeof()
        peer_size = tadd.tadd_ipv4_peer_sizeof()

        self.tadd_mem = create_string_buffer(tadd_size)
        self.peers_mem = create_string_buffer(peer_size * len(peers))
        self.callback = callback
        self.recv_cb = c_recv_cb_type(self.callback_ll)
        self.send_cb = c_send_cb_type(0)

        rc = tadd.tadd_ipv4_init(self.tadd_mem, c_char_p(addr.encode()), port,
            self.peers_mem, len(peers))
        if rc != 0:
            raise Exception()

        for i, peer in enumerate(peers):
            rc = tadd.tadd_ipv4_peer_add(self.tadd_mem, pointer(
                c_char.from_buffer(self.peers_mem, i * peer_size)),
                c_char_p(peer[0].encode()), peer[1],
                self.recv_cb, 0,
                self.send_cb, 0)
            if rc != 0:
                raise Exception()
            rc = tadd.tadd_send_rst(self.tadd_mem, i)
            if rc != 0:
                raise Exception()

    def callback_ll(self, ctadd, data, closure):
        src_id = tadd.tadd_recv_src_id(ctadd, data)
        size = tadd.tadd_recv(ctadd, 0, 0, 0, data)
        buf = create_string_buffer(size)
        rc = tadd.tadd_recv(ctadd, buf, 0, size if size else 1, data)
        if (rc == size):
            self.callback(buf, src_id)
        else:
            raise Exception()
        return 0

    def idle(self):
        return tadd.tadd_idle_blocking(self.tadd_mem)

    def send(self, dst_id, data):
        tadd.tadd_idle(self.tadd_mem)
        data_mem = (c_char * len(data)).from_buffer_copy(data)
        return tadd.tadd_send(self.tadd_mem, c_size_t(dst_id), data_mem,
            c_size_t(len(data)))

if __name__ == "__main__":
    import sys

    if (len(sys.argv) < 5 or len(sys.argv) > 6):
        print("Takes 4 or 5 arguments")
        exit()
    addr = sys.argv[1]
    port = int(sys.argv[2])

    peer_addr = sys.argv[3]
    peer_port = int(sys.argv[4])

    def cb(data, src_id):
        print(str(data, "utf-8"))

    t = tadd_ipv4(addr, port, [(peer_addr, peer_port)], cb)

    if (len(sys.argv) == 6): # send
        t.send(0, bytes(sys.argv[5], "utf-8"))
        while (True):
            data = input()
            t.send(0, bytes(data, "utf-8"))
    else:
        while (True):
            t.idle()
