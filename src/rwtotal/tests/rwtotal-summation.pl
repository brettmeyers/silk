#! /usr/bin/perl -w
# MD5: ec9bca64377be77d3cb7cacc2620c348
# TEST: ./rwtotal --sport --summation --skip-zero ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --sport --summation --skip-zero $file{data}";
my $md5 = "ec9bca64377be77d3cb7cacc2620c348";

check_md5_output($md5, $cmd);
