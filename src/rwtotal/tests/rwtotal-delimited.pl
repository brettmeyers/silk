#! /usr/bin/perl -w
# MD5: d52de0391f8feccc5dffbff16cde627a
# TEST: ./rwtotal --sport --delimited --skip-zero ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --sport --delimited --skip-zero $file{data}";
my $md5 = "d52de0391f8feccc5dffbff16cde627a";

check_md5_output($md5, $cmd);
