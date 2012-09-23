#! /bin/bash

MOUNT_PATH=/mnt

CLIENT_CNT=10 # the max number of client nodes
# DEFAULT: the number of VMs on each client is 15
FANOUT=5
FILECNT=5

mkdir $MOUNT_PATH/META_FS.vmr
for ((i=0;i<$CLIENT_CNT;++i))
do
  mkdir $MOUNT_PATH/vmr.vm$i
  for ((j=0;j<2;++j))
  do
    mkdir $MOUNT_PATH/vm$i.vm$i$j
    for ((k=0;k<2;++k))
    do
      mkdir $MOUNT_PATH/vm$i$j.vm$i$j$k
      for ((l=0;l<2;++l))
      do
        mkdir $MOUNT_PATH/vm$i$j$k.vm$i$j$k$l
        for ((m=0;m<2;++m))
        do    
          mkdir $MOUNT_PATH/vm$i$j$k$l.vm$i$j$k$l$m
        done
      done
    done
  done
done

./mkdir_eval.o $MOUNT_PATH/vmr $FANOUT > /dev/null
./file_eval.o -c $MOUNT_PATH/vmr $FANOUT $FILECNT > /dev/null

