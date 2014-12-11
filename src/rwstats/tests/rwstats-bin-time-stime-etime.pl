#! /usr/bin/perl -w
# MD5: f9f8cd6ac64143b47b1fc91c66a36d1f
# TEST: ./rwstats --fields=stime,etime --bin-time=3600 --values=bytes,packets,flows --count=500 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=stime,etime --bin-time=3600 --values=bytes,packets,flows --count=500 $file{data}";
my $md5 = "f9f8cd6ac64143b47b1fc91c66a36d1f";

check_md5_output($md5, $cmd);
