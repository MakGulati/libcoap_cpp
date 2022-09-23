# Makefile for libcoap minimal examples
#
# Copyright (C) 2018-2021 Olaf Bergmann <bergmann@tzi.org>
#

LIBCOAP?=libcoap-3

pkgconfig=$(shell pkg-config $(1) $(2))
CPPFLAGS=-Wall -Wextra $(call pkgconfig,--cflags,$(LIBCOAP)) -shared libnanocbor.so
LDLIBS=$(call pkgconfig,--libs,$(LIBCOAP))
LINK.o=$(LINK.cc) 
# LINK.o=-shared: build/libnanocbor.so

CXXFLAGS=-std=c++14

all: client server

client:: client.o common.o

server:: server.o common.o

clean:
	rm -f *.o client server
