#!/usr/bin/perl -w
#mkdir testenv testenv/dir testenv2 testenv2/dir2; touch testenv/dir/impfile ; 
#ln -s `pwd`/testenv/dir/impfile testenv # tylko w dol; 
#ln -s `pwd`/testenv/dir/impfile testenv2/dir2
#ln -s dir/impfile testenv/impfile2; ln -s ../../testenv/dir/impfile testenv2/dir2/impfile2
#ls -lR
$numArgs = $#ARGV + 1;
print "You provided $numArgs arguments\n";
$ARGV[0]="testenv";
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

find(\&wanted, $ARGV[0]);
sub wanted { 
	if (-l $_) {
		$nme=cwd().'/'.$_;
		$too=readlink $_;
		$too=realpath($_);
		$_ = "Capes:Geoff::Shot putter:::Big Avenue";
		@fds = split(/\//, $too);
		print convertToRelative($nme, $too);
		print join(';',@fds)."\n";
		print $too."\n";
		print "cze: ".$nme."\n\n";
	}
}
