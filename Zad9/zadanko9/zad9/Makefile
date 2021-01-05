EXECS = myping

CPPFLAGS = -Wall
LDFLAGS	= 
LDLIBS = 

TARGET: $(EXECS)

myping.o err.o dropnobody.o : err.h

myping: myping.o err.o in_cksum.o dropnobody.o

.PHONY: clean TARGET
clean:
	rm -f $(EXECS) *.o *~ 
