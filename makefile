#rules definition

all : libnetfiles myserver

myserver : myserver.c libnetfiles.h 
	$(CC) $(CCFLAGS) -pthread -o myserver myserver.c

libnetfiles: libnetfiles.c libnetfiles.h
	$(CC) $(CCFLAGS) -o libnetfiles libnetfiles.c

tester : libnetfiles.c libnetfiles.h myserver.c tester.c
	$(CC) $(CCFLAGS) -o tester tester.c libnetfiles.c

#This rule cleans up executable file

clean: 
	rm -f myserver libnetfiles
