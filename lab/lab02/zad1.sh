#!/bin/sh
if [ $# -le 0 ]
then
echo "nie ma argumentu!"
exit 1
fi

rm m1 m2 l1 l2 l3 l4 l5 l6
touch m1 m2
#rm -f l* 
ln m1 l1 
ln m1 l2 
ln m1 l3
ln m2 l4
ln m2 l5 
read -p "Press [Enter] key to perform action of removing uneccessary hard links in directory $i."
if [ ! -d $1 ]
then
echo "to nie katalog"
exit 2
fi


ls -l1i

cd $1
fls=`ls .`

for f in $fls 
do
inod=`ls -i $f | sed 's/\([0-9]*\).*/\1/'`
ktore="pierwsze"
	for i in `ls .` 
	do	
		thisinode=`ls -i $i | sed 's/\([0-9]*\).*/\1/'`
		if [ "$inod" = "$thisinode" ]; then
			if [ $ktore = "pierwsze" ]; then
			echo "mamy pierwszy duplikat $1/$f = $1/$i"
			ktore="drugie+"
			else
			echo "mamy n-ty duplikat $f = $i"
			path=`$i`
			rm -f $i 
			ln -s $f $i
			fi
		fi
	done
echo $f $inode
done
cd -

#dla zadanych dwoch katalogow znalezc pliki regularne wystepujace w obu

