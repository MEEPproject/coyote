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

sub usage()
{

  print "\n";
  print " Usage: matmul_gendata.pl [options] \n";
  print "\n";
  print " Options:\n";
  print "  --help  print this message\n";
  print "  --size  size of input data [1000]\n";
  print "  --seed  random seed [1]\n";
  print "$usageMsg";

  exit();
}

sub processCommandLine()
{

  $opts{"help"} = 0;
  $opts{"size"} = 1000;
  $opts{"seed"} = 1;
  Getopt::Long::GetOptions( \%opts, 'help|?', 'size:i', 'seed:i' ) or usage();
  $opts{"help"} and usage();

}

#--------------------------------------------------------------------------
# Helper Functions
#--------------------------------------------------------------------------

sub printArray
{
  my $arrayName = $_[0];
  my $arrayRef  = $_[1];
  my $type = $_[2];
  my $size = $_[3];

  my $numCols = 20;
  my $arrayLen = scalar(@{$arrayRef});

  print "static ".$type." ".$arrayName."[".$size."] = \n";
  print "{\n";

  if ( $arrayLen <= $numCols ) {
    print "  ";
    for ( my $i = 0; $i < $arrayLen; $i++ ) {
      print sprintf("%3d",$arrayRef->[$i]);
      if ( $i != $arrayLen-1 ) {
        print ", ";
      }
    }
    print "\n";
  }

  else {
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



#--------------------------------------------------------------------------
# Matmul
#--------------------------------------------------------------------------

# http://answers.oreilly.com/topic/418-how-to-multiply-matrices-in-perl/

sub mmult {
    my ($m1,$m2) = @_;
    my ($m1rows,$m1cols) = matdim($m1);
    my ($m2rows,$m2cols) = matdim($m2);

    my $result = [  ];
    my ($i, $j, $k);

    for $i (range($m1rows)) {
        for $j (range($m2cols)) {
            for $k (range($m1cols)) {
                $result->[$i][$j] += $m1->[$i][$k] * $m2->[$k][$j];
            }
        }
    }
    return $result;
}

sub range { 0 .. ($_[0] - 1) }


sub veclen {
    my $ary_ref = $_[0];
    my $type = ref $ary_ref;
    if ($type ne "ARRAY") { die "$type is bad array ref for $ary_ref" }
    return scalar(@$ary_ref);
}

sub matdim {
    my $matrix = $_[0];
    my $rows = veclen($matrix);
    my $cols = veclen($matrix->[0]);
    return ($rows, $cols);
}

sub init_matrix{

}

#--------------------------------------------------------------------------
# Main
#--------------------------------------------------------------------------

sub main()
{

  processCommandLine();
  srand($opts{"seed"});

  # create random input arrays

  my $n=$opts{"size"};
  my $ia;
  my $ja;
  my $a;

  my $row = 0;
  my $nnz = 0;

  for my $i (range($n)){
    for my $j (range($n)){
      $ia->[$row] = $nnz;
      if ($i > 0) {
        $ja->[$nnz] = $row - $n;
        $a->[$nnz] = -1.0;
        $nnz++;
      }
      if ($j > 0) {
        $ja->[$nnz] = $row - 1;
        $a->[$nnz] = -1.0;
        $nnz++;
      }
      $ja->[$nnz] = $row;
      $a->[$nnz] = 4.0;
      $nnz++;
      if ($j < $n - 1) {
        $ja->[$nnz] = $row + 1;
        $a->[$nnz] = -1.0;
        $nnz++;
      }
      if ($i < $n - 1) {
        $ja->[$nnz] = $row + $n;
        $a->[$nnz] = -1.0;
        $nnz++;
      }
      $row++;
    }
  }
  $ia->[$row] = $nnz;

  my $x;
  for my $i (range($nnz)){
    $x->[$i] = 1.0;
  }

  print "\n#ifndef __DATASET_H";
  print "\n#define __DATASET_H";
  print "\n\#define DIM ".$opts{"size"}." \n\n";
  print "\n\#define N_ROWS DIM*DIM \n\n";
  print "\n\#define N_NON_ZERO ".$nnz." \n\n";
   
  printArray( "ia", $ia, "long", "N_ROWS+1");
  printArray( "ja", $ja, "long", "N_NON_ZERO");
  printArray( "a", $a, "double", "N_NON_ZERO");
  printArray( "x", $x, "double", "N_NON_ZERO");

  print "\n#endif //__DATASET_H";
 
}

main();

