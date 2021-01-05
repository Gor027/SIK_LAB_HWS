TARGETS = time-server time-client

CC     = gcc
CFLAGS = -Wall -O2
LFLAGS = -Wall

all: $(TARGETS)

time-server.o time-client.o err.o: err.h

time-server: time-server.o err.o

time-client: time-client.o err.o

.PHONY: clean

clean:
	rm -f $(TARGETS) *.o *~ *.bak
