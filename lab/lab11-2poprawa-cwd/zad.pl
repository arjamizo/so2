#!/usr/bin/perl -w
#mkdir testenv testenv/dir testenv2 testenv2/dir2; touch testenv/dir/impfile ; 
#ln -s `pwd`/testenv/dir/impfile testenv # tylko w dol; 
#ln -s `pwd`/testenv/dir/impfile testenv2/dir2
#ln -s dir/impfile testenv/impfile2; ln -s ../../testenv/dir/impfile testenv2/dir2/impfile2
#ls -lR

#w zadanym drzewie katalogow, sposrob tych do ktorych wskazuje wiecej niz 3, to wsytzksie poza pirewszym przerobic tak, aby wskazywaly na pirwszy

#uzycie: tar -xf testenv.tar.gz -C testenv && ls -lR . && ./scr.pl && ls -lR . && rm -r testenv
#uzycie: 
#./init.sh install; ls -lR; ./zad.pl > /dev/null; ls -lR ; ./init.sh uninstall; 

$numArgs = $#ARGV + 1;
print "You provided $numArgs arguments\n";
$ARGV[0]="../lab10-poprawa-cwd";
$ARGV[0]=".";
$CWD=$ARGV[0];
$CWD=realpath($CWD);
print "Working in directory $ARGV[0]\n";

use Cwd; 
use Cwd 'realpath'; 
use File::Find;
use File::stat;


@fnms=(); # nazwa pliku na ktory wskazuje dowiazanie
@occurences=(); #wartosc licznika wystepujacych elementow
@firstoccur=(); #pirewsze wystapienie
@symlinksEnds=(); #endpoints of symlinks, punkty koncowe symlinkow, jakoby wspolny mianownik wszystkich skrotow

sub addoccurence {
	local($fnme, $value) = ($_[0], realpath($_[0]));
	for ($i=0; $i<scalar(@fnms); $i++) {
		if ($value eq $fnms[$i]) {
			print "found symlink to $value (pointed from '$fnme') \n";
			$occurences[$i]++;
			print "".($occurences[$i]-1)."=>".$occurences[$i]."\n";
			return;
		}
	}
	print "adding $value\n";
	push(@firstoccur, $fnme);
	push(@fnms, $value);
	push(@symlinksEnds, realpath(readlink($fnme)));
	push(@occurences, 1);
}
sub getnumberofoccurences {
	local($value,$abs) = ($_,realpath($_));
	#print "returning number of occurences of $value = ";
	for ($i=0; $i<scalar(@fnms); $i++) {
		#print "comparing $value ($abs) eq $fnms[$i]\n";
		if ($abs eq $fnms[$i]) {
			#print "FUOND ".$occurences[$i]."\n";
			return $occurences[$i];
		}
	}
	#print "\n";
}

sub indexOf {
	local($value,@array) = (@_);
	print "looking for $value in @array\n";
	for ($i=0; $i<scalar(@array); $i++) {
		if ($value eq $symlinksEnds[$i]) {
			return $i;
		}
	}
	return -1;
}



sub printOccurences {
	for ($i=0; $i<scalar(@fnms); $i++) {
		print "link to ".($fnms[$i])." exists ".$occurences[$i].", first link = $firstoccur[$i]\n";
	}
}

sub listallsymlinks {
	if (-l $_) {
		addoccurence($_);
		print $_."\n";
	}
}


find (\&listallsymlinks, $ARGV[0]);
printOccurences();

#na te chwile mamy w tablicach fnms, occurences i firstoccur wszystkie potrzebne dane do daleszeg odzialania

print "fnms=@fnms\n";
find (\&handleAllEntries, $ARGV[0]);
sub handleAllEntries {
	print "not a link" && return if (! -l $_);
	local($fnme) = ($_);
	#print "handling $_\n";
	$a=realpath($fnme);
	$abs=$a;
	if (getnumberofoccurences($a)<3) {
	print "was done before or not enough existing links\n";
	return;
	};
	$index=indexOf($a,@symlinksEnds);
	if ($fnme eq $firstoccur[$index]) {print "first occur!\n"; return;}
	unlink($fnme);
	symlink($firstoccur[$index], $fnme);
	print "plik ".$fnme." zostal obsluzony\n";
	print "if file ".$a." is in {".join(",",@symlinksEnds)."}[".$index."] then copy occ from {@symlinksEnds}[$index] to $fnme\n";
	print "changed poiting '$fnme' to '".$firstoccur[$index]."'\n";
}
