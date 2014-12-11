#! /usr/bin/perl -w
# MD5: b17a4ee049eb1d6f7570eba04e63742c
# TEST: cat ../../tests/data.rwf | ./rwcombine --buffer-size=1m --max-idle-time=0.002 --output-path=/dev/null --print-statistics=stdout

use strict;
use SiLKTests;

my $rwcombine = check_silk_app('rwcombine');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "cat $file{data} | $rwcombine --buffer-size=1m --max-idle-time=0.002 --output-path=/dev/null --print-statistics=stdout";
my $md5 = "b17a4ee049eb1d6f7570eba04e63742c";

check_md5_output($md5, $cmd);
