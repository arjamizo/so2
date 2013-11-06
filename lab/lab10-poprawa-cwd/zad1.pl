#!/usr/bin/perl -w
#mkdir testenv testenv/dir testenv2 testenv2/dir2; touch testenv/dir/impfile ; 
#ln -s `pwd`/testenv/dir/impfile testenv # tylko w dol; 
#ln -s `pwd`/testenv/dir/impfile testenv2/dir2
#ln -s dir/impfile testenv/impfile2; ln -s ../../testenv/dir/impfile testenv2/dir2/impfile2
#ls -lR
#jeśli plik ma wiecej niz 1 i mniej niz 3 lub 3 dowiazania symboliczne prowadzace do niego, to niech wszystkie prowadza do pierwszego z nich? (lub skopiuj je gdzies? nie pamietam tresci pytania)

#uzycie: ./init.sh && ls -lR; ./zad1.pl && ls -lR; ./init.sh uninstall
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

sub convertToRelative {
	local($path1, $path2) = ($_[0], $_[1]);
	print "Fighting $path1 against $path2"."\n";
	@path1=split(/\//, $path1);
	@path2=split(/\//, $path2);
	if (scalar(@path1)>scalar(@path2)) {
		@t=@path1;
		@path1=@path2;
		@path2=@t;
		#(@path1,@t)=(@path2,@path1);
	}
	print join(';',@path1)."<-PIERWSZY\n";
	print join(':',@path2)."<-DRUGI\n";
	#from now on it is sure that path1 is shorter
	$i=0; 
	#finding common part
	#for my $i (1 .. $#path1) {
	for ($i=1; $i<$#path1; $i++) {
		if ( $path1[$i] ne $path2[$i] ) {last;}
    		print "ARRAY $i: $path1[$i] == $path2[$i] \n";
	}
	$i=$i-1;
	print $i."\n";
	print "@path1".":::"."@path2"."\n";
	print @path1[(scalar(@path1)-$i) .. -1]; print "\n";
	print @path2[(scalar(@path2)-$i) .. -1]; print "\n";
	print "END\n";
	return "KONIEC\n";
}

#(\&wanted, $ARGV[0]);
@intfiles=('cze');
#print "".join (",",@intfiles)."\n";
#exit;

$n=0;
$path2="";
sub counter {
	#local($path1, $path2) = ($_[0], $_[1]);
	print "\t".realpath($_)." eq $path2\n";
	if (realpath($_) eq $path2) {
		$n=$n+1;
	}
}

sub replaceFiles {
	print "if file $_ is in ".join(",",@intfiles)." then copy occ from @intfiles to $_\n";
}

sub wanted { 
	$too=realpath($_);
	#print $too."\n";
	#$stat=stat($_);
	$canon=File::Spec->canonpath($too);
	$fnme=cwd().'/'.$_;

		$n=0;
		$path2=$fnme;
		print "CWD=".realpath($ARGV[0])."\n";
		find(\&counter, realpath($CWD));
		$n=$n-1;
		#print $n."\n";
		#.stat($_)->nlink." ".$too." ; $canon"
		print "do pliku $fnme jest $n dowiazan."."\n"; #if $n>1 && $n<=3;
		push (@intfiles, $fnme) if $n>1 && $n<=3;	
}

find (\&listallsymlinks, $ARGV[0]);

@fnms=();
@occurences=();
sub addoccurence {
	local($value) = ($_[0]);
	for ($i=0; $i<scalar(@fnms); $i++) {
		if ($value eq $fnms[$i]) {
			print "found $value\n";
			$occurences[$i]++;
			return;
		}
	}
	print "adding $value\n";
	push(@fnms, $value);
	push(@occurences, 0);
}
sub getnumberofoccurences {
	local($value) = ($_[0]);
	for ($i=0; $i<scalar(@fnms); $i++) {
		if ($value eq $fnms[$i]) {
			return $occurences[$i];
		}
	}
}

sub printOccurences {
	for ($i=0; $i<scalar(@fnms); $i++) {
		print $fnms[$i]." exists ".$occurences[$i]."\n";
	}
}

sub listallsymlinks {
	if (-l $_) {
		addoccurence(realpath($_));
		print $_."\n";
	}
}
print scalar(@fnms)."HERE\n";
printOccurences();
