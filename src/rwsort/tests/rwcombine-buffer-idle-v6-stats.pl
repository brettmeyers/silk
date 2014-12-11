#! /usr/bin/perl -w
# MD5: b17a4ee049eb1d6f7570eba04e63742c
# TEST: ./rwcombine --buffer-size=2m --max-idle-time=0.002 --output-path=/dev/null --print-statistics=stdout ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwcombine = check_silk_app('rwcombine');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
my $cmd = "$rwcombine --buffer-size=2m --max-idle-time=0.002 --output-path=/dev/null --print-statistics=stdout $file{v6data}";
my $md5 = "b17a4ee049eb1d6f7570eba04e63742c";

check_md5_output($md5, $cmd);
