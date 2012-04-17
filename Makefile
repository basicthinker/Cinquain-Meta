CC = gcc
CFLAGS = -std=gnu99 -g -pthread -Wall -DCINQ_DEBUG -DHASH_DEBUG=1
#CFLAGS = -std=gnu99 -Wall -O3
LIB = -lpthread
OBJDIR = Objects
SRC := $(wildcard *.c)
OBJ := $(SRC:%.c=$(OBJDIR)/%.o)

all : dir test

test: $(OBJ)
	$(CC) $(CFLAGS) $(LIB) -o $@ $^

$(OBJDIR)/%.o : %.c
	$(CC) -c -MMD $(CFLAGS) -o $@ $<

-include $(OBJDIR)/*.d

dir :
	mkdir -p $(OBJDIR)

clean :
	rm -rf $(OBJDIR) test
