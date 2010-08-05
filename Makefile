CFLAGS=-Wall -O0 -g

all: avg

avg: avg.c
	$(CC) $(CFLAGS) -o $@ $<

.PHONY:all clean
clean:
	rm -f avg
