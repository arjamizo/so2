#!/usr/bin/perl
#! mkdir a b c ; touch a/a.txt a/b.txt a/c.txt.jpeg.exe ; dd if=/dev/zero of=a/movie.avi  bs=1M  count=1 ; dd if=/dev/zero of=a/a.txt  bs=1M  count=1 ; chmod g+x a/a.txt ; chmod g+x a/*.txt*exe
#wypisz wszystkie pliki w danym podkatalogu, ktore moga byc wykonane, maja tylko jedno rozszerzenie i sa wieksze niz podany rozmiar
#uruchamianie ./zad1.pl KATALOG
use File::Find;
use File::stat;
use Cwd; 
use Fcntl ':mode';
use feature qw(say);
print "LOOKING BELOW: ".$ARGV[0]."\n";
find(\&wanted, $ARGV[0]);
sub wanted { 

	$nme=cwd().'/'.$_;
	$allowed=S_IMODE(S_IXUSR|S_IXGRP|S_IXOTH);
	$perm=((stat($nme)->mode)& 07777) & $allowed;
	#printf "%s =mode> %04o\n", $nme, $perm;
	#$regexp=/^[^\.]+$/;
	#say $regexp;
	#$allowbyname=($_ =~ $regexp);
	#say $_."\t\tresult=".($allowbyname);
	if ($_ =~ /^\.?[^\.]+\.?[^\.]+$/ && (stat($nme)->mode & S_IMODE(S_IXUSR|S_IXGRP|S_IXOTH) ) && stat($nme)->size>=10*1024) {
		#say stat($nme)->size." bytes";
		say "ALLOWED FILE: ".$nme;
	}
}

