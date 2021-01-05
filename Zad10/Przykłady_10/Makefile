CC = gcc
CFLAGS = -Wall 
TARGETS = le-client le-server 

all: $(TARGETS) 

le-server.o le-client.o err.o: err.h

le-client: le-client.o err.o
	$(CC) $(CFLAGS) $^ -o $@ -levent

le-server: le-server.o err.o
	$(CC) $(CFLAGS) $^ -o $@ -levent

clean:
	rm -f *.o $(TARGETS) 
