#! /usr/bin/perl -w
# MD5: fbab4b85c63316bcfc365f424ed5b560
# TEST: ./rwcount --bin-size=3600 --load-scheme=0 --end-time=2009/02/14T19:30:00 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=3600 --load-scheme=0 --end-time=2009/02/14T19:30:00 $file{data}";
my $md5 = "fbab4b85c63316bcfc365f424ed5b560";

check_md5_output($md5, $cmd);
