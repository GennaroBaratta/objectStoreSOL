CC			= gcc
AR			= ar
CFLAGS		+= -std=c99 #-Wall -Wextra -pedantic -g -DMAKE_VALGRIND_HAPPY -fsanitize=thread
ARFLAGS		= rvs
INCLUDES	= -I.
LDFLAGS		= -L.
OPTFLAGS	= -O3
LIBS		= -pthread

TARGETS		= server \
			  client

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

libstore.a: $(OBJECTS)
		$(AR) $(ARFLAGS) $@ $^
	
clean:
		rm -f $(TARGETS) objstore.sock
		rm -rf data/

cleanall: clean
	\rm -f *.o *~ libstore.a

test:
	make cleanall
	make all
	./server -&
	./test.sh 	
	killall -w server
	@echo "**** test superato"