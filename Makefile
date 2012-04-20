CC = gcc
EXTRA_CFLAGS = -std=gnu99 -g -pthread -Wall -DCINQ_DEBUG -DHASH_DEBUG=1
# EXTRA_CFLAGS = -std=gnu99 -pthread -Wall -Os
LIB = -lpthread
OBJDIR = obj
KSRC = /lib/modules/`uname -r`/build
SRC := $(wildcard *.c)
OBJ := $(SRC:%.c=$(OBJDIR)/%.o)

obj-m := cinqfs.o
cinqfs-objs := $(OBJ)

all : dir test

modules: all
	$(MAKE) -C $(KSRC) M=`pwd`

test: $(OBJ)
	$(CC) $(EXTRA_CFLAGS) $(LIB) -o $@ $^

$(OBJDIR)/%.o : %.c
	$(CC) -c -MMD $(EXTRA_CFLAGS) -o $@ $<

-include $(OBJDIR)/*.d

dir :
	mkdir -p $(OBJDIR)

clean :
	rm -rf $(OBJDIR) test
	$(MAKE) -C $(KSRC) M=`pwd` clean
