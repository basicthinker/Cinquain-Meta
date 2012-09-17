CC = gcc
CFLAGS = -Wall
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

all : $(OBJ)
%.o : %.c
	gcc $(CFLAGS) -o $*.o $*.c

clean:
	rm *.o
