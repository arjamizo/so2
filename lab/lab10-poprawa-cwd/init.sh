#!/bin/sh

touch file
ln -s file ln1
ln -s file ln2
ln -s file ln3
mkdir dir
touch dir/fle
ln -s dir/fle ln6
ln -s dir/fle ln7
ln -s dir/fle ln8

echo "$1"
if [ "$1" = "uninstall" ]
then
rm -r dir
rm ln*
rm file
fi
