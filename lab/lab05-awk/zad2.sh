#!/bin/sh

#w zadanym drzewie katalogow znaleźć pliki regularne, ktorych nazwa jest zawarta w nazwie katalogu w ktorym wystepuja

folder=$1

find $folder -type f | awk -F '/' '
( $(NF-1) ) ~ $(NF)  { print $0 }
'
