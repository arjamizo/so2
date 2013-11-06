#!/bin/sh

#w zadanym drzewie katalogow znaleźć pliki regularne, ktorych nazwa jest zawarta w nazwie katalogu w ktorym wystepuja

folder=.

if [ ! true ]
then
mkdir aba bab
touch aba/b
touch bab/a
mkdir a.txt
touch a.txt/a.txt a.txt/a
fi

##moznaby jeszcze uwzglednic sciezki zawierajace .
find . -type f | awk -F '/' '
isany=0; 
for (i = 1; i <= NF; i++) if ( ( $(NF-1) ) ~ $(NF) ) isany = isany + 1;
isany { print $0 }
'
