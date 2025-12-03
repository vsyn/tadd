from subprocess import Popen
from shutil import which

gccpath = which("gcc")
p = Popen([gccpath, "-g3", "-O1", "-Wall", "tests/tadd_multi_test.c",
    "tadd.c", "tadd_ipv4.c", "tadd_recv.c", "tadd_send.c", "tadd_socket.c",
    "tadd_peer.c", "lblist/lblist.c", "lbtree/lbtree.c",
    "lbtree/examples/lbtree_uint.c", "everand/everand.c", "-Ilbtree",
    "-Ilblist", "-Ilbtree/examples", "-Ieverand", "-Itadd_pool"])
rc = p.wait()
if (rc):
    exit()

vgpath = which("valgrind")
ps = [Popen([vgpath, "--tool=memcheck", "--main-stacksize=33554432",
    "--leak-check=full", "--show-leak-kinds=all", "./a.out", str(i)])
    for i in range(4)]
for p in ps:
    p.wait()

p = Popen(["rm", "./a.out"])
p.wait()