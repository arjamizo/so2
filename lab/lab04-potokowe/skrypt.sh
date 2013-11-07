#!/bin/sh
#chmod +x skrypt.sh
#mkdir a b c d && touch a/ala b/ala c/jan d/jan ala
#w zadanym drzewie katlogow znaleźc pliki regularne i katalogi tak samo sie nazywajace
if [ $# -ne 1 -o ! -d $1 ]; then 
echo "uruchomienie $0 sciezka_do_katalogu"
exit
fi
#sort -t " "
find $1  \( \( -type d -printf '%f %p d\n' \) -or \( -type f -printf '%f %p f\n' \) \) | sort -d | 
( 
last=""
buf=""
read f fp ft
while read f2 fp2 ft2
do
	#echo "$a $a2" # => $f $f2
	if [ $f = $f2 -a $ft != $ft2 ]; then 
		buf="$buf $fp $fp2"
		last=$f2
		#${ft}:$fp ${ft2}:$fp2
	fi
	#echo "$f2 $last $buf"
	if [ "$f2" != "$last" -a -n "$buf" ] ; then 
		echo "duplikat nazwy: $f => `echo \"$buf\" | sort | uniq`"
	buf=""
	fi
	f=$f2
	fp=$fp2
	last=$f2
done
)
