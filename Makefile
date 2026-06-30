CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2
LDFLAGS ?= -lm

.PHONY: all run test test-shell clean

all: amosoz

amosoz: amosoz.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

run: amosoz
	./amosoz

test: amosoz
	printf 'selftest\nexit\n' | ./amosoz

test-shell: amosoz
	sh tests/shell_treaty.sh

clean:
	rm -f amosoz amosoz_neo amosoz.img