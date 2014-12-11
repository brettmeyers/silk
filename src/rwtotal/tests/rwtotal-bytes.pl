#! /usr/bin/perl -w
# MD5: f33c353360f81230b41e8c5cc286cff0
# TEST: ./rwtotal --bytes --skip-zero ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --bytes --skip-zero $file{data}";
my $md5 = "f33c353360f81230b41e8c5cc286cff0";

check_md5_output($md5, $cmd);
