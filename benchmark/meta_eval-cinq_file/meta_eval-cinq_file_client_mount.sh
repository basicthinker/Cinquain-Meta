#! /bin/bash

PATH_PRE='/root/remote'
CLIENT_NUM=0 # each client node has a number between 0 - 9
MDS_IP=localhost
# DEFAULT: the number of VMs on each client is 15
FANOUT=5
FILECNT=5

  for ((j=0;j<2;++j))
  do
    for ((k=0;k<2;++k))
    do
      for ((l=0;l<2;++l))
      do
        for ((m=0;m<2;++m))
        do    
          mkdir -p $PATH_PRE/c$CLIENT_NUM-v$j$k$l$m
          mount -t nfs $MDS_IP:/c$CLIENT_NUM-v$j$k$l$m $PATH_PRE/c$CLIENT_NUM-v$j$k$l$m
        done
      done
    done
  done
wait
