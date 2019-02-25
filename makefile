CC=gcc
CFLAGS=-Wall -g
OBJ=envelop.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

envelop: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

