#! /usr/bin/perl -w
# MD5: b17a4ee049eb1d6f7570eba04e63742c
# TEST: ./rwcombine ../../tests/data.rwf ../../tests/empty.rwf --max-idle-time=0.002 --output-path=/dev/null --print-statistics=stdout

use strict;
use SiLKTests;

my $rwcombine = check_silk_app('rwcombine');
my %file;
$file{data} = get_data_or_exit77('data');
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwcombine $file{data} $file{empty} --max-idle-time=0.002 --output-path=/dev/null --print-statistics=stdout";
my $md5 = "b17a4ee049eb1d6f7570eba04e63742c";

check_md5_output($md5, $cmd);
