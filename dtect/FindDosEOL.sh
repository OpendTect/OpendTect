#!/bin/sh

files=`cat $1 | xargs grep -l $'\r'`
if [ -z "$files" ];then
   echo "No DOS EOL found!"
else
   echo "DOS EOL found in: "
   echo $files
   exit 1
fi

