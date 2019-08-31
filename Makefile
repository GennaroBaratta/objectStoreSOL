CC			= gcc
AR			= ar
CFLAGS		+= -std=c99 -Wall -Wextra -pedantic -g -DMAKE_VALGRIND_HAPPY -fsanitize=address
ARFLAGS		= rvs
INCLUDES	= -I.
LDFLAGS		= -L.
OPTFLAGS	= -Og
LIBS		= -pthread

TARGETS		= server \
			  client \
			  imgclient

OBJECTS		= clientLibrary.o \
				communication.o \
				icl_hash.o \
				request.o

INCLUDE_FILES	= clientLibrary.h \
				communication.h \
				icl_hash.h \
				request.h
				  

.PHONY: all clean cleanall

.SUFFIXES: .c .h

%: %.c
		$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS)

%.o: %.c
		$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -c -o $@ $<

all: $(TARGETS)

server: server.o libstore.a
		$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

client: client.o libstore.a
		$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

imgclient: imgclient.o libstore.a
		$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)


libstore.a: $(OBJECTS)
		$(AR) $(ARFLAGS) $@ $^
	
clean:
		rm -f $(TARGETS) objstore.sock *.o *~ libstore.a testout.log
		rm -rf data/

testInternal:
	make clean
	make all
	./server -&
	./test.sh 
	./imgclient	 >> testout.log 
	./testsum.sh
	killall -w server
	@echo "**** test superato"

test:
	./test.sh
	@echo "**** test superato"