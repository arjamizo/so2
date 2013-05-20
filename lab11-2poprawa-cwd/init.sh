if [ $# -lt 1 ] 
then
	echo too less parameters
echo usage: init.sh - installs testenvironment
echo init.sh uninstall - uninstalls test environment
	exit
fi

if [ $1 == 'uninstall' ] 
then
rm ln* cod doc ocd
exit
fi

touch cod doc ocd
ln -s cod lncod1
ln -s cod lncod2
ln -s cod lncod3
ln -s doc lndoc1
ln -s doc lndoc2
ln -s doc lndoc3
ln -s ocd lnocd1
ln -s ocd lnocd2
ln -s ocd lnocd3
ln -s ocd lnocd4
ln -s ocd lnocd5
