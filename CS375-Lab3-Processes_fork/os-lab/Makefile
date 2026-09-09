CC      := gcc
CFLAGS  := -Wall -Wextra -std=gnu11 -g
BINDIR  := bin

# One executable per .c file in src/ and challenges/ (each has its own main).
SRC_PROGS  := $(patsubst src/%.c,$(BINDIR)/%,$(wildcard src/*.c))
CH_PROGS   := $(patsubst challenges/%.c,$(BINDIR)/%,$(wildcard challenges/*.c))
PROGS      := $(SRC_PROGS) $(CH_PROGS)

all: $(PROGS)

$(BINDIR)/%: src/%.c | $(BINDIR)
	$(CC) $(CFLAGS) $< -o $@

$(BINDIR)/%: challenges/%.c | $(BINDIR)
	$(CC) $(CFLAGS) $< -o $@

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -rf $(BINDIR)

.PHONY: all clean
