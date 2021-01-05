TARGETS = multi-recv multi-send multi-recv6 multi-send6

CC     = gcc
CFLAGS = -Wall -O2
LFLAGS = -Wall

all: $(TARGETS)

multi-recv.o multi-send.o multi-recv6.o multi-send6.o err.o: err.h

multi-recv: multi-recv.o err.o

multi-send: multi-send.o err.o

multi-recv6: multi-recv6.o err.o

multi-send6: multi-send6.o err.o

.PHONY: clean

clean:
	rm -f $(TARGETS) *.o *~ *.bak
