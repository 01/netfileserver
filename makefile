#Variable definition

CC = gcc
CCFLAGS = -Wall

#rules definition

all : libnetfiles netfileserver

libnetfiles: libnetfiles.h libnetfiles.c tester.c
	$(CC) $(CCFLAGS) -o client libnetfiles.c tester.c

netfileserver: libnetfiles.h netfileserver.c
	$(CC) $(CCFLAGS) -pthread -o server netfileserver.c

#This rule cleans up executable file

clean: 
	rm -f client server
