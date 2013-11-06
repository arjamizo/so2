#!/usr/bin/perl
# w zadanym drzewie katalogow znalexc dowiazania symboliczne wskazujace na plik regularny ktory sie nazywa tak samo jak dowiazanie 
#mkdir -p zad2env/folder; touch zad2env/a.txt zad2env/folder/a.txt
#uruchamianie: zad2.pl zad2env
use File::Find;
use File::stat;
use Cwd; 
use Fcntl ':mode';
use Cwd 'abs_path';
use File::Basename;

use feature qw(say);
print "LOOKING BELOW: ".$ARGV[0]."\n";
find(\&wanted, $ARGV[0]);
sub wanted { 
	$nme=cwd().'/'.$_;
	$abs=abs_path($nme);
	($name,$path,$suffix) = fileparse($abs,@suffixlist);
	if(-f $abs && -l $nme && $name==$_) {
		say cwd().'/'.$_.' -> '.$abs;
	}
}
