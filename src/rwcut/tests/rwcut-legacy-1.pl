#! /usr/bin/perl -w
# MD5: b29a675b055728afc5eb3d3ba818f1b9
# TEST: ./rwcut --fields=9,11 --timestamp-format=m/d/y,no-msec ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=9,11 --timestamp-format=m/d/y,no-msec $file{data}";
my $md5 = "b29a675b055728afc5eb3d3ba818f1b9";

check_md5_output($md5, $cmd);
