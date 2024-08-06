CC = gcc
CFLAGS = -g -std=gnu11 -Werror -Wall -Wextra -Wpedantic -Wmissing-declarations -Wmissing-prototypes -Wold-style-definition

mexec: mexec.o
	$(CC) $(CFLAGS) mexec.o -o mexec
mexec.o: mexec.c
	$(CC) $(CFLAGS) -c mexec.c -o mexec.o

clean:
	rm -f mexec.o
