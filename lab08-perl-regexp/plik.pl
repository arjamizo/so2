#!/usr/bin/perl -w
#na tablicy: $zm=(@tabs=~s/...(...)(...).../$1$2/g # LUB s[ LUB m/ LUB s#

# W zadanym pliku zastąpić liczby naturalne przez liczby postaci L.0
# Nie zast. liczb ujemnych, liczb. sklejonych z tesktsem.
# Założyć, że pomiędzy liczbą a znakiem przystankowym zawsze jest spacja.

# Print the value of the command line arguments
$numArgs = $#ARGV + 1;
print "You provided $numArgs arguments\n";
print "Input file is $ARGV[0]\n";
print "Output file is $ARGV[1]\n\n";

# Open input file in read mode
open INPUTFILE, "<", $ARGV[0] or die $!;
# Open output file in write mode
#open OUTPUTFILE, ">", $ARGV[1] or die $!;

# Read the input file line by line

sub dne() {
$val=($_ =~ s/(\s|^)(\d+)(\s)/$1$2\.0$3/csgxmi);
print $val.":> ";
return $val;
}

while (<INPUTFILE>) {

  #print $_ =~ m/(\s)(\d+)([\s])/g;
#while( $_ =~ s/(\d+)/$1\.0/gx){};
  while (dne()){};
  ($ARGV[1] and print(OUTPUTFILE $_)) or print($_);
}

close INPUTFILE;
close OUTPUTFILE;
