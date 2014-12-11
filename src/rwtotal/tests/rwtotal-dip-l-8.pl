#! /usr/bin/perl -w
# MD5: dd4ff48793bb14d14e1b9a75b9dd419e
# TEST: ./rwtotal --dip-last-8 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --dip-last-8 $file{data}";
my $md5 = "dd4ff48793bb14d14e1b9a75b9dd419e";

check_md5_output($md5, $cmd);
