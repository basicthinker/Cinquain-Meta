CC = gcc
CFLAGS = -Wall -DCINQ_DEBUG -DHASH_DEBUG=1 -O3
LIB = -lpthread
OBJDIR = Objects
SRC := $(wildcard *.c)
OBJ := $(SRC:%.c=$(OBJDIR)/%.o)

all : dir test

test: $(OBJ)
	$(CC) $(CFLAGS) $(LIB) -o $@ $^

$(OBJDIR)/%.o : %.c
	$(CC) -c -MMD $(FLAGS) -o $@ $<

-include $(OBJDIR)/*.d

dir :
	mkdir -p $(OBJDIR)

clean :
	rm -r $(OBJDIR) test
