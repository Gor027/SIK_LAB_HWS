TARGET: echo-server echo-client

CC	= gcc
CFLAGS	= -Wall -O2
LFLAGS	= -Wall

echo-server.o echo-client.o err.o: err.h

echo-server: echo-server.o err.o
	$(CC) $(LFLAGS) $^ -o $@

echo-client: echo-client.o err.o
	$(CC) $(LFLAGS) $^ -o $@

.PHONY: clean TARGET
clean:
	rm -f echo-server echo-client *.o *~ *.bak
