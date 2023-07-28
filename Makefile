# prgi: Makefile
#
# Copyright (C) 2023  Joao Luis Meloni Assirati.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

CFLAGS=-O3

examples := $(patsubst %.c,%,$(wildcard example_*.c))

.DEFAULT_GOAL := all

all: $(examples)

# This example needs threads
example_threads: example_threads.c prgi.c prgi.h
	gcc -Wall $(CFLAGS) -pthread -o $@ $< prgi.c -lm

# These examples don't need threads. In this case, prgi.c doesn't need
# to be cmpiled with -pthread.
example_%: example_%.c prgi.c prgi.h
	gcc -Wall $(CFLAGS) -o $@ $< prgi.c -lm

clean:
	rm -f $(examples) *.o

cleanbak: clean
	rm -f *~

.PHONY: all clean cleanbak
