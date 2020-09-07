COMPILER = cc
CFLAGS = -std=c99 -Wall
LDFLAGS = -lsodium
default: citpass

citpass:
	$(COMPILER) -o citpass $(CFLAGS) main.c $(LDFLAGS)

install:
	cp citpass /usr/bin/
	chmod 755 /usr/bin/citpass
	cp citpass.1 /usr/local/share/man/man1
	chmod 644 /usr/local/share/man/man1/citpass.1

.PHONY: clean
clean:
	rm citpass
