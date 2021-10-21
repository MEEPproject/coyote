#!/usr/bin/perl -w
#==========================================================================
# matmul_gendata.pl
#
# Author : Christopher Batten (cbatten@mit.edu)
# Date   : April 29, 2005
#
(our $usageMsg = <<'ENDMSG') =~ s/^\#//gm;
#
# Simple script which creates an input data set and the reference data
# for the matmul benchmark.
#
ENDMSG

use strict "vars";
use warnings;
no  warnings("once");
use Getopt::Long;

#--------------------------------------------------------------------------
# Command line processing
#--------------------------------------------------------------------------

our %opts;

sub usage() {

  print "\n";
  print " Usage: gendata.pl [options] \n";
  print "\n";
  print " Options:\n";
  print "  --help  print this message\n";
  print "  --size  size of input data [1000]\n";
  print "  --seed  random seed [1]\n";
  print "$usageMsg";

  exit();
}

sub processCommandLine() {

  $opts{"help"} = 0;
  $opts{"size"} = 1000;
  $opts{"seed"} = 1;
  Getopt::Long::GetOptions( \%opts, 'help|?', 'size:i', 'seed:i' ) or usage();
  $opts{"help"} and usage();

}

#--------------------------------------------------------------------------
# Helper Functions
#--------------------------------------------------------------------------

sub printArray {
  my $arrayName = $_[0];
  my $arrayRef  = $_[1];
  my $type = $_[2];
  my $size = $_[3];

  my $numCols = 20;
  my $arrayLen = scalar(@{$arrayRef});
  
  print "static ".$type." ".$arrayName."[".$size."] __attribute__((aligned (1024)))= \n";
  print "{\n";

  if ( $arrayLen <= $numCols ) {
    print "  ";
    for ( my $i = 0; $i < $arrayLen; $i++ ) {
      print sprintf("%3d", $arrayRef->[$i]);
      if ( $i != $arrayLen-1 ) {
        print ", ";
      }
    }
    print "\n";
  } else {
    my $numRows = int($arrayLen/$numCols);
    for ( my $j = 0; $j < $numRows; $j++ ) {
      print "  ";
      for ( my $i = 0; $i < $numCols; $i++ ) {
        my $index = $j*$numCols + $i;
        print sprintf("%3d",$arrayRef->[$index]);
        if ( $index != $arrayLen-1 ) {
          print ", ";
        }
      }
      print "\n";
    }

    if ( $arrayLen > ($numRows*$numCols) ) {
      print "  ";
      for ( my $i = 0; $i < ($arrayLen-($numRows*$numCols)); $i++ ) {
        my $index = $numCols*$numRows + $i;
        print sprintf("%3d",$arrayRef->[$index]);
        if ( $index != $arrayLen-1 ) {
          print ", ";
        }
      }
      print "\n";
    }

  }
  print  "};\n\n";
}


sub range { 0 .. ($_[0] - 1) }




#--------------------------------------------------------------------------
# Main
#--------------------------------------------------------------------------

sub main() {

  processCommandLine();
  srand($opts{"seed"});

  # create random input arrays

  my $n=$opts{"size"};
  
  my $matrix;
  my $indices;
  my $vector;
  
  
  for my $i (range($n)) {
    $indices->[$i] = ($i * 8) % ($n*$n);  # create some indices, which cannot be cached
    $vector->[$i] = $i;
  }

  for my $i (range($indices->[-1])) {
    $matrix->[$i] = $i;
  }

  print "\n#ifndef __DATASET_H";
  print "\n#define __DATASET_H";
  print "\n\#define DIM ".$opts{"size"}." \n\n";
   
  printArray( "matrix", $matrix, "long", "(DIM-1)*8");
  printArray( "indices", $indices, "long", "DIM");
  printArray( "vector", $vector, "double", "DIM");

  print "\n#endif //__DATASET_H";
 
}

main();
