COMPILER = gcc
CFLAGS = -std=c99
default: citpass
citpass:
	$(COMPILER) -o citpass $(CFLAGS) main.c
.PHONY: clean
clean:
	rm citpass
	rm *.o
