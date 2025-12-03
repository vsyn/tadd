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

#ifndef TADD_SOCKET_H
#define TADD_SOCKET_H

#include "tadd.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

int tadd_socket_get_timeout(int sock, const struct timeval *timeout);
int tadd_socket_set_timeout(int sock, const struct timeval *timeout);
int tadd_socket_send(int sock, struct sockaddr *sockaddr, socklen_t socklen,
                     struct tadd_data_linked *data);
int tadd_socket_recv(struct tadd *tadd, int sock, struct sockaddr *sockaddr,
                     socklen_t *socklen, struct tadd_data_linked **dout,
                     unsigned char blocking);
int tadd_socket_init(int domain, struct sockaddr *sockaddr, socklen_t socklen);

#endif
