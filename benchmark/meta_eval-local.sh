#! /bin/bash

PATH_PRE='/mnt/vm'
VM_CNT=10

FANOUT=6
FILECNT=5
REPEAT=2

for ((r=0; r<$REPEAT; ++r))
do

  # Evaluate mkdir
  for (( i=0; i<$VM_CNT; ++i ))
  do
    ./mkdir_eval.o "$PATH_PRE$i" $FANOUT &
  done
  wait

  for (( i=0; i<$VM_CNT; ++i ))
  do
    ./file_eval.o -c "$PATH_PRE$i" $FANOUT $FILECNT &
  done
  wait

  for (( i=0; i<$VM_CNT; ++i ))
  do
    ./file_eval.o -o "$PATH_PRE$i" $FANOUT $FILECNT &
  done
  wait

  tree "$PATH_PRE$((VM_CNT-1))" > check.tree

  for (( i=0; i<$VM_CNT; ++i ))
  do
    ./file_eval.o -r "$PATH_PRE$i" $FANOUT $FILECNT &
  done
  wait

  for (( i=0; i<$VM_CNT; ++i ))
  do
    ./rmdir_eval.o "$PATH_PRE$i" $FANOUT &
  done
  wait
done

