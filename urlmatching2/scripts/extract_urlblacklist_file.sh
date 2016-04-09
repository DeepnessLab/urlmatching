#!/bin/bash

if [ -z "$1" ]
  then
    echo "Error: tar file"
	exit 1
fi

mkdir tmp
tar -C /tmp -xvf $1
cd tmp/blacklists
cat `find .` >> blacklists.txt
strings blacklists.txt > blacklists_clean.txt
mv  blacklists_clean.txt ../../
cd ../../

echo "Done - $1 was imported into blacklists_clean.txt"
echo "Please delete tmp folder"
