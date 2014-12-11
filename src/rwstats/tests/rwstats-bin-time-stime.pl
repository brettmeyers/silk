#! /usr/bin/perl -w
# MD5: 12579d4e773d8af6290b0dea4e552e64
# TEST: ./rwstats --fields=stime --bin-time=3600 --values=packets --count=100 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=stime --bin-time=3600 --values=packets --count=100 $file{data}";
my $md5 = "12579d4e773d8af6290b0dea4e552e64";

check_md5_output($md5, $cmd);
