CFLAGS = -O2

out/spoofproxy.so: spoofproxy.c
	$(CC) -ansi -Wall -Wextra -Wpedantic -Wint-conversion -Wshadow -Werror -shared -fPIC $(CFLAGS) $< -o $@

clean:
	if [ -f out/spoofproxy.so ] ; then rm out/spoofproxy.so ; fi

.PHONY: clean
