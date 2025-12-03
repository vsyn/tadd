#Copyright 2023 Julian Ingram
#
#Licensed under the Apache License, Version 2.0(the "License");
#you may not use this file except in compliance with the License.
#You may obtain a copy of the License at
#
#http: // www.apache.org/licenses/LICENSE-2.0
#
#Unless required by applicable law or agreed to in writing, software
#distributed under the License is distributed on an "AS IS" BASIS,
#WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#See the License for the specific language governing permissions and
#limitations under the License.

DEFINES =
INCLUDES = lbtree lblist tadd_pool lbtree/examples

CC := gcc
AR := ar
CFLAGS += -g3 -O0 -Werror -Wall -Wextra $(DEFINES:%=-D%) $(INCLUDES:%=-I%)
#expanded below
DEPFLAGS = -MMD -MP -MF $(@:$(BUILD_DIR)/%.o=$(DEP_DIR)/%.d)
LDFLAGS := -g3 -O0
SRCS := \
	lbtree/lbtree.c \
	lbtree/examples/lbtree_uint.c \
	lblist/lblist.c \
	tadd_peer.c \
	tadd_recv.c \
	tadd_send.c \
	tadd.c \
	tadd_socket.c \
	tadd_ipv4.c
TEST_DIR := tests
TEST_SRCS := \
	$(SRCS) \
	tests/tadd_test.c
#sort removes duplicates
ALL_SRCS := $(sort $(SRCS) $(TEST_SRCS))
BIN_DIR ?= bin
TARGET ?= $(BIN_DIR)/libtadd.a
SHARED_TARGET ?= $(BIN_DIR)/libtadd.so
TEST ?= $(BIN_DIR)/tadd_test
RM := rm -rf
MKDIR := mkdir -p
CP := cp -r
#BUILD_DIR and DEP_DIR should both have non - empty values
BUILD_DIR ?= build
DEP_DIR ?= $(BUILD_DIR)/deps
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
SHARED_OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.pic)
TEST_OBJS := $(TEST_SRCS:%.c=$(BUILD_DIR)/%.o)
EXAMPLE_OBJS := $(EXAMPLE_SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS := $(ALL_SRCS:%.c=$(DEP_DIR)/%.d)

.PHONY: all
all: $(TARGET)

#link lib
$(TARGET): $(OBJS)
	$(if $(BIN_DIR),$(MKDIR) $(BIN_DIR),)
	$(AR) -rcsD $@ $^

.PHONY: shared
shared: $(SHARED_TARGET)

$(SHARED_TARGET): $(SHARED_OBJS)
	$(if $(BIN_DIR),$(MKDIR) $(BIN_DIR),)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

.PHONY: test
test: $(TEST)
	./$(BIN_DIR)/tadd_test

#link test
$(TEST): $(TEST_OBJS)
	$(if $(BIN_DIR),$(MKDIR) $(BIN_DIR),)
	$(CC) -o $@ $^ $(LDFLAGS)

#compile and / or generate dep files
$(BUILD_DIR)/%.o: %.c
	$(MKDIR) $(BUILD_DIR)/$(dir $<)
	$(MKDIR) $(DEP_DIR)/$(dir $<)
	$(CC) $(DEPFLAGS) $(CFLAGS) -c $< -o $@

#compile and / or generate dep files
$(BUILD_DIR)/%.pic: %.c
	$(MKDIR) $(BUILD_DIR)/$(dir $<)
	$(MKDIR) $(DEP_DIR)/$(dir $<)
	$(CC) $(DEPFLAGS) $(CFLAGS) -c -fPIC $< -o $@

.PHONY: clean
clean:
	$(RM) $(TARGET) $(TEST) $(BIN_DIR) $(DEP_DIR) $(BUILD_DIR)

-include $(DEPS)
