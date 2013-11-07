#!/bin/sh
wid=12
file=text.txt
if [ $# -ge 1 ]; then file=$1; fi;
if [ $# -ge 2 ]; then wid=$2; fi;
cat $file | awk -F '\t' '
BEGIN {}
    function max(m, n) {
       return (m > n ? m : n)
    }
    function min(m, n) {
       return (m < n ? m : n)
    }
{
shouldbe=0
is=0
debug=""
   for(i=1; i<=NF; i++) {
       shouldbe+='"$wid"'
       printf($i)
       printf("")
       spacenum='"$wid"'-length($i)
       is+=length($i)+max(-spacenum,0)
       #printf(">sb=" shouldbe ", is=" is ", snum=" spacenum "<")
       debug=debug "|is=" is ", sb=" shouldbe "|\t"
       #if (is < shouldbe)
           for(j=is; j<shouldbe; j++) {printf(" ")};
       printf(" ")
   }
   print "\n"
   printf (debug "\n")
}'
