

CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror

default: aesdsocket
	
all: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) $(CFLAGS) aesdsocket.o -o aesdsocket 

aesdsocket.o:
	$(CC) $(CFLAGS) aesdsocket.c -c

clean:
	rm -f *.o aesdsocket *.elf *.map

	
