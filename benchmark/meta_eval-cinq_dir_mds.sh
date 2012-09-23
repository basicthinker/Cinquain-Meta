#! /bin/bash

MOUNT_PATH=/mnt
VM_CNT=150 # number of client nodes * number of VMs on each client

for ((i=0;i<$VM_CNT;++i))
do
  mkdir $MOUNT_PATH/META_FS.vm$i
done
