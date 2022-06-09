#!/bin/bash
GXEMUL=/OSLAB/gxemul
RUN_FILE=/home/git/20373696/gxemul/vmlinux
if [ "$1" = "run" ];then
	$GXEMUL -E testmips -C R3000 -M 64 $RUN_FILE
elif [ "$1" = "debug" ];then
	$GXEMUL -E testmips -C R3000 -M 64 $RUN_FILE -V
elif [ "$1" = "file_run" ];then
	$GXEMUL -E testmips -C R3000 -M 64 -d /home/git/20373696/gxemul/fs.img $RUN_FILE
else
	echo "wrong command format"
fi
