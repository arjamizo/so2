#!/bin/sh
if [ $# -le 0 ]
then
echo "nie ma katalogu!"
exit 1
fi

if [ -d $1 ] #oraz prawa zapisu do tego 
then
echo "to nie katalog"
exit 2
fi

rm -f $1/*.old #2>/dev/null
#if !ls then; fi
#chmod -w zad1.sh
#touch 1 2 3 && chmod -w 1 2 3 &&  touch 4 5


pliki=`ls ${1}`
echo $pliki
for i in $pliki 
do 
 	echo "sprawdzam $1/$i"
	if [ -f $1/$i  ]; then
		mv $1/$i $1/$i.old
	fi 
	
done
#chmod +w zad1.sh

#uruchamiac: ./zad1.sh, a pozniej $?

#w zad katalog utworzyc podkatalogi wedlug nazwe zawartychw pliku tekstowym
# jezeli konflikt to w pliku zostawic taka nazwe
