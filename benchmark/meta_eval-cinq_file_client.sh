#! /bin/bash

PATH_PRE='/mnt/vm'
CLIENT_NUM=3 # each client node has a number between 0 - 9
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
          ./file_eval.o -o $PATH_PRE$CLIENT_NUM$j$k$l$m &
        done
      done
    done
  done
wait
