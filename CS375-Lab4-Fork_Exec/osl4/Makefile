CC      := gcc
CFLAGS  := -Wall -Wextra -std=gnu11 -g
BINDIR  := bin

SRCS  := $(wildcard *.c)
PROGS := $(patsubst %.c,$(BINDIR)/%,$(SRCS))

all: $(PROGS)

$(BINDIR)/%: %.c | $(BINDIR)
	$(CC) $(CFLAGS) $< -o $@

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -rf $(BINDIR)

.PHONY: all clean
