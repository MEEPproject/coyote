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
  my $N = $_[4];

  my $numCols = 20;
  my $arrayLen = scalar(@{$arrayRef});


  print "static ".$type." ".$arrayName."[".$size."] = \n";
  print "{\n";

  for my $a (range(3)){
    for my $i (range($N)){
      for my $j (range($N)){
        for my $k (range($N)){
            print "  ";
            print sprintf("%3d",$arrayRef->[$a][$i][$j][$k]);
            if($i ne $N-1 or $j ne $N-1 or $k ne $N-1 or $a ne 2)
            {
                print ","
            }
        }
      }
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

  my $N=$opts{"size"};
  my $X;
  my $V;
  my $A;
  my $F;
  
  my $Xcenter;
 
  $Xcenter->[0]=0;
  $Xcenter->[1]=0; 
  $Xcenter->[2]=0;

  for my $i (range($N)){
    for my $j (range($N)){
      for my $k (range($N)){
        $X->[0][$i][$j][$k] = $i;
        $X->[1][$i][$j][$k] = $j;
        $X->[2][$i][$j][$k] = $k;
        
	    $Xcenter->[0] += $X->[0][$i][$j][$k];
        $Xcenter->[1] += $X->[1][$i][$j][$k];
        $Xcenter->[2] += $X->[2][$i][$j][$k];

        $V->[0][$i][$j][$k] = 0;
        $V->[1][$i][$j][$k] = 0;
        $V->[2][$i][$j][$k] = 0;
        

        $A->[0][$i][$j][$k] = 0;
        $A->[1][$i][$j][$k] = 0;
        $A->[2][$i][$j][$k] = 0;

        $F->[0][$i][$j][$k] = 0;
        $F->[1][$i][$j][$k] = 0;
        $F->[2][$i][$j][$k] = 0;
      }
    }
  }
  $Xcenter->[0] /= ($N*$N*$N);
  $Xcenter->[1] /= ($N*$N*$N);
  $Xcenter->[2] /= ($N*$N*$N);

  print "\n#ifndef __DATASET_H";
  print "\n#define __DATASET_H";
  print "\n\#define N ".$opts{"size"}." \n\n";
   
  print "static double Xcenter[3] = {".$Xcenter->[0].", ".$Xcenter->[1].", ".$Xcenter->[2]."};\n\n";
  printArray( "X", $X, "double", "N*N*N*3", $N);
  printArray( "V", $V, "double", "N*N*N*3", $N);
  printArray( "A", $A, "double", "N*N*N*3", $N);
  printArray( "F", $F, "double", "N*N*N*3", $N);

  print "\n#endif //__DATASET_H";
 
}

main();

