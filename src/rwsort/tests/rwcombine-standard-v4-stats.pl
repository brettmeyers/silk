#! /usr/bin/perl -w
# MD5: 5ebe66497ce2d847d03717b63603daa5
# TEST: ./rwcombine ../../tests/empty.rwf ../../tests/data.rwf --output-path=/dev/null --print-statistics=stdout

use strict;
use SiLKTests;

my $rwcombine = check_silk_app('rwcombine');
my %file;
$file{empty} = get_data_or_exit77('empty');
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcombine $file{empty} $file{data} --output-path=/dev/null --print-statistics=stdout";
my $md5 = "5ebe66497ce2d847d03717b63603daa5";

check_md5_output($md5, $cmd);
