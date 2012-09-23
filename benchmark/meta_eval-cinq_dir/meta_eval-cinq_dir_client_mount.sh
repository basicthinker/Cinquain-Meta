#! /bin/bash

PATH_PRE='/root/remote'
CLIENT_NUM=0 # current number of client node, any one from 0 - 9
MDS_IP=localhost
VM_CNT=15

FANOUT=5
FILECNT=5

  # Evaluate mkdir
  for (( i=0; i<$VM_CNT; ++i ))
  do
    mkdir -p $PATH_PRE/c$CLIENT_NUM-v$i
    mount -t nfs $MDS_IP:/c$CLIENT_NUM-v$i $PATH_PRE/c$CLIENT_NUM-v$i
  done

