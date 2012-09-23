#! /bin/bash

PATH_PRE='/root/remote'
CLIENT_NUM=0 # current number of client node, any one from 0 - 9
VM_CNT=15

FANOUT=5
FILECNT=5

  # Evaluate mkdir
  for (( i=0; i<$VM_CNT; ++i ))
  do
    ./mkdir_eval.o "$PATH_PRE/c$CLIENT_NUM-v$i" $FANOUT &
  done
  wait

  tree "$PATH_PRE/c$CLIENT_NUM-v$((VM_CNT-1))" > check.tree

  for (( i=0; i<$VM_CNT; ++i ))
  do
    ./rmdir_eval.o "$PATH_PRE/c$CLIENT_NUM-v$i" $FANOUT &
  done
  wait

