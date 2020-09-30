COMPILER = cc
CFLAGS = -std=c99 -Wall -Wpedantic -Wextra
LDFLAGS = -lsodium
default: citpass

citpass:
	$(COMPILER) $(CFLAGS) main.c -o citpass $(LDFLAGS)

install:
	cp citpass /usr/bin/
	chmod 755 /usr/bin/citpass
	cp citpass.1 /usr/local/share/man/man1
	chmod 644 /usr/local/share/man/man1/citpass.1

.PHONY: clean
clean:
	rm citpass
