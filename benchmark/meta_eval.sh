#! /bin/bash

PATH_PRE='/mnt/vm'
VM_CNT=4

FANOUT=5
REPEAT=1

for (( i=0; i<$VM_CNT; ++i ))
do
  ./meta_eval "$PATH_PRE$i" $FANOUT $REPEAT &
done
wait
