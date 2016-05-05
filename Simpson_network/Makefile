#CC		= clang-3.5
CC		= gcc
WARNINGS	= -Wall
STANDART	= 
THREADS		= -pthread
MATH		= -lm
#================================================
SPECIAL		= 
#================================================
CFLAGS		=  $(STANDART) $(WARNINGS) $(THREADS) $(SPECIAL) -MD
.PHONY: all clean gcov
#================================================
all: client.out server.out

client.out: client_main.c client_api.o
	$(CC) $(CFLAGS) client_api.o client_main.c -o client.out

client_api.o: client_api.c
	$(CC) $(CFLAGS) client_api.c -c -o client_api.o

server.out: server_main.c server_api.o
	$(CC) $(CFLAGS) server_api.o server_main.c -o server.out

server_api.o: server_api.c
	$(CC) $(CFLAGS) server_api.c -c -o server_api.o

clean:
	rm -rf *.o *.d *.gcda *.gcno *.gcov *.preproc client.out server.out

-include $(wildcard *.d)