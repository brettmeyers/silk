#! /usr/bin/perl -w
# MD5: dd9100d5e4079e63c2767a69c37ac4d3
# TEST: ./rwstats --fields=stime,etime,dur --bin-time=3600 --values=bytes,packets,flows --count=500 ../../tests/data.rwf 2>/dev/null

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=stime,etime,dur --bin-time=3600 --values=bytes,packets,flows --count=500 $file{data} 2>/dev/null";
my $md5 = "dd9100d5e4079e63c2767a69c37ac4d3";

check_md5_output($md5, $cmd);
