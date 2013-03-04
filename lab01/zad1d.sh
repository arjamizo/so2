#!/bin/sh

if [ $# -le 0 ]
then
echo "nie ma katalogu!"
exit 1
fi

if [ -d $# ] #oraz prawa zapisu do tego folderu
then
echo "to nie katalog"
exit 2
fi

#echo "a b c d e f">foldery.txt
nazwy=`cat foldery.txt`
#echo $nazwy

errors=""

for n in $nazwy 
do
	if [ ! -d $1/$n ]
	then 
		mkdir $1/$n
	else
		echo "blad dla $1/$n"
		errors="${errors} ${n}"
		#lub: echo $n >> plik.txt, a przed wszystkim rm plik.txt
	fi
done

echo $errors | tee foldery_err.txt 











#http://www.wykop.pl/link/330010/10-komend-bash-ktore-ulatwia-ci-zycie/

