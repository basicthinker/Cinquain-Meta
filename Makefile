CC = gcc
CFLAGS = -std=c99 -Wall -DCINQ_DEBUG -DHASH_DEBUG=1 -O3
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
	rm -r $(OBJDIR) test
