.PHONY: clean default

CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -pedantic -g

default: hello
hello: hello.c
clean:
	rm -f hello
