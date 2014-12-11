#! /usr/bin/perl -w
# MD5: 9cbfe7cd2e989c63f28012fc2edfba7d
# TEST: ./rwtotal --dip-last-16 --skip-zero ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --dip-last-16 --skip-zero $file{data}";
my $md5 = "9cbfe7cd2e989c63f28012fc2edfba7d";

check_md5_output($md5, $cmd);
