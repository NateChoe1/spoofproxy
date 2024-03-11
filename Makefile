CFLAGS = -O2
spoofproxy.so: spoofproxy.c
	$(CC) -ansi -Wall -Wextra -Wpedantic -Wint-conversion -Wshadow -Werror -shared -fPIC $(CFLAGS) $< -o $@
