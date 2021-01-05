CC = gcc
CFLAGS = -Wall
TARGETS = client proc_server thread_server preforked_server 

all: $(TARGETS) 

err.o: err.c err.h

client.o: client.c err.h

client: client.o err.o

thread_server.o: thread_server.c err.h

thread_server: thread_server.o
	$(CC) $(CFLAGS) $^ -o $@ -lpthread err.o

proc_server.o: proc_server.c err.h

proc_server: proc_server.o err.o

preforked_server.o: preforked_server.c err.h

preforked_server: preforked_server.o err.o

clean:
	rm -f *.o *~ $(TARGETS) 
