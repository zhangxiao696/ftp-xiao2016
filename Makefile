.PHONY:clean
CC=gcc
CFLAGS=-Wall -g
BIN=my_ftp
OBJS=main.o socket_operate.o session.o service_process.o worker_process.o string_operate.o \
parameter.o para_operate.o internal_sock.o hash.o
LIBS=-lcrypt

$(BIN):$(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)
%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -f *.o $(BIN)
