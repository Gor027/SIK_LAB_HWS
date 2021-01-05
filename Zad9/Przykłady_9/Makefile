EXECS = rawping spoofping

CPPFLAGS = -Wall
LDFLAGS	= 
LDLIBS = 

TARGET: $(EXECS)

rawping.o err.o dropnobody.o spoofping.o: err.h

rawping: rawping.o err.o in_cksum.o dropnobody.o

spoofping: spoofping.o err.o in_cksum.o dropnobody.o

.PHONY: clean TARGET
clean:
	rm -f $(EXECS) *.o *~ 
