#! /bin/bash

PATH_PRE='/mnt/vm'
VM_CNT=4

FANOUT=5
REPEAT=2

for ((r=0; r<$REPEAT; ++r))
do

  # Evaluate mkdir
  for (( i=0; i<$VM_CNT; ++i ))
  do
    ./mkdir_eval.o "$PATH_PRE$i" $FANOUT &
  done
  wait

done

