#! /usr/bin/perl -w
# MD5: d66e14a9704fa9e48c69f22c0bc488d9
# TEST: ./rwtotal --sport --min-record=10 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --sport --min-record=10 $file{data}";
my $md5 = "d66e14a9704fa9e48c69f22c0bc488d9";

check_md5_output($md5, $cmd);
