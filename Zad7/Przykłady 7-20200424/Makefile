CC = gcc
CFLAGS = -Wall
TARGETS = client poll_server poll_server_telnet

all: $(TARGETS) 

err.o: err.c err.h

client.o: client.c err.h

client: client.o err.o

poll_server.o: poll_server.c err.h

poll_server: poll_server.o err.o

poll_server_telnet.o: poll_server_telnet.c err.h

poll_server_telnet: poll_server_telnet.o err.o

clean:
	rm -f *.o $(TARGETS) 
