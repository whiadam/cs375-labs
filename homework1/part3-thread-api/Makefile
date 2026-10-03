PROGS = main-race main-race-removed main-race-lock-one main-race-lock-both main-deadlock main-deadlock-global main-signal main-signal-cv

all: $(PROGS)

%: %.c common_threads.h
	gcc -o $@ $< -Wall -pthread -g

clean:
	rm -f $(PROGS)
