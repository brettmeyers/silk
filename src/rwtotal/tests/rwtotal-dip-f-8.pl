#! /usr/bin/perl -w
# MD5: 1ce223cdc3af0e06beca9f91736a9aeb
# TEST: ./rwtotal --dip-first-8 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --dip-first-8 $file{data}";
my $md5 = "1ce223cdc3af0e06beca9f91736a9aeb";

check_md5_output($md5, $cmd);
