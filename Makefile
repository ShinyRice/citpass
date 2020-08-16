COMPILER = gcc
CFLAGS = -std=c99
default: citpass

citpass:
	$(COMPILER) -o citpass $(CFLAGS) main.c

install:
	cp citpass /usr/bin/
	chmod 755 /usr/bin/citpass

.PHONY: clean
clean:
	rm citpass
#	rm *.o
