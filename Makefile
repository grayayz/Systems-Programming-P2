CC = gcc
CFLAGS = -Wall -Wextra -g -fsanitize=address,undefined
LDFLAGS = -lm

compare: compare.c
	$(CC) $(CFLAGS) -o compare compare.c $(LDFLAGS)

clean:
	rm -f compare
