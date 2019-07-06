CC = clang
CFLAGS = -Wall -Werror -O2 -ggdb

main: main.c cpu.c cache.c mem.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	-rm -rf main main.dSYM

.PHONY: clean
