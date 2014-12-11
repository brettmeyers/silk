#! /usr/bin/perl -w
# MD5: 5a8b30d40553414bf44fe065962faf2b
# TEST: ./rwtotal --sip-last-16 --skip-zero ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --sip-last-16 --skip-zero $file{data}";
my $md5 = "5a8b30d40553414bf44fe065962faf2b";

check_md5_output($md5, $cmd);
