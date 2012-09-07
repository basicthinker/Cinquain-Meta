ccflags-y += -Os # -DCINQ_DEBUG -DHASH_DEBUG=1
KSRC = /lib/modules/`uname -r`/build

obj-m += cinqfs.o
cinqfs-objs := cinq_meta.o cnode.o file.o fsnode.o super.o cinq_cache/rbtree.o cinq_cache/cinq_cache.o

default:
	$(MAKE) -C $(KSRC) M=`pwd`
install:
	insmod cinqfs.ko
	mkdir -p /mnt
	mount -t cinqfs none /mnt
uninstall:
	umount /mnt
	rmmod cinqfs
clean:
	$(MAKE) -C $(KSRC) M=`pwd` clean
