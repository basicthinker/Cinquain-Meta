#! /bin/bash

PATH_PRE='/home/basicthinker/Projects/cinquain-meta/mnt/vm'
ENUM_CNT=1

FANOUT=6
REPEAT=1

for (( i=0; i<$ENUM_CNT; ++i ))
do
  mkdir -p "$PATH_PRE$i"
done

for (( i=0; i<$ENUM_CNT; ++i ))
do
  ./meta_eval "$PATH_PRE$i" $FANOUT $REPEAT
done
