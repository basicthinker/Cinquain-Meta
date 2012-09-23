#! /bin/bash

MOUNT_PATH=/mnt
CLIENT_CNT=10
VM_CNT=15

for ((i=0;i<$CLIENT_CNT;++i))
do
  for ((j=0;j<$VM_CNT;++j))
  do
    mkdir $MOUNT_PATH/META_FS.c$i-v$j
  done
done
