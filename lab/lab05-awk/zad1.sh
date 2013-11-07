#!/bin/sh
#w zadanym drzewie katalogow wypisac wszystkie pliki bez rozszerzenia

folder=.

mkdir a b
touch f1.txt f1 f2 f3.avi f4.mpeg
cp f* a
cp f* b

find $folder -type f | awk -F '/' '$NF !~ /.*\..*/ { print $0 }'
