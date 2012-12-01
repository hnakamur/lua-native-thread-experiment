SYS := $(shell gcc -dumpmachine)

HEADERS= \
  src/nativethread.h \
  src/nativethread-priv.h \

OBJS= \
  src/nativethread.o \

TARGET_BASENAME=nativethread
LIBUV_DIR=deps/uv
LIBUV_A=$(LIBUV_DIR)/libuv.a

CC=gcc
CFLAGS += -g
CFLAGS += -Iinclude -Iinclude/couv-private -I$(LIBUV_DIR)/include
LDFLAGS += -L$(LIBUV_DIR) -luv

LIBUV_MAKE_OPT=
ifneq (, $(findstring linux, $(SYS)))
TARGET=$(TARGET_BASENAME).so
LUA_E=luajit
LIBUV_MAKE_OPT += CFLAGS=-fPIC
CFLAGS += -fPIC -D_XOPEN_SOURCE=500 -D_GNU_SOURCE
CFLAGS += --std=c89 -pedantic -Wall -Wextra -Wno-unused-parameter
LDFLAGS += -shared -Wl,-soname,$(TARGET) -lpthread -lrt -lm
LDFLAGS += -llua
else ifneq (, $(findstring darwin, $(SYS)))
TARGET=$(TARGET_BASENAME).so
LUA_E=lua
CFLAGS += --std=c89 -pedantic -Wall -Wextra -Wno-unused-parameter
LDFLAGS += -llua
LDFLAGS += -lpthread -bundle -undefined dynamic_lookup -framework CoreServices
else ifneq (, $(findstring mingw, $(SYS)))
TARGET=$(TARGET_BASENAME).dll
LUA_E=luajit
CFLAGS += --std=gnu89 -D_WIN32_WINNT=0x0600 -I/usr/local/include/luajit-2.0
LDFLAGS += -L/usr/local/bin -llua51
LDFLAGS += -shared -Wl,--export-all-symbols -lws2_32 -lpsapi -liphlpapi
endif


all: $(TARGET)

$(TARGET): $(LIBUV_A) $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

$(LIBUV_A):
	cd $(LIBUV_DIR) && make $(LIBUV_MAKE_OPT)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

src/nativethread.o: src/nativethread.c $(HEADERS)

.PHONY: test clean

test: $(TARGET)
	$(LUA_E) tool/checkit test/test-*.lua

clean:
	-cd $(LIBUV_DIR) && make clean
	-rm -f $(TARGET) $(OBJS)
