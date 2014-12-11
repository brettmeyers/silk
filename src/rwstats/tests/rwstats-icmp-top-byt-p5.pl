#! /usr/bin/perl -w
# MD5: 1ab88fdfd843ee83d95d5da0dcde866f
# TEST: ../rwfilter/rwfilter --proto=1 --pass=- ../../tests/data.rwf | ./rwstats --fields=icmpTypeCode --byte --percentage=5

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --proto=1 --pass=- $file{data} | $rwstats --fields=icmpTypeCode --byte --percentage=5";
my $md5 = "1ab88fdfd843ee83d95d5da0dcde866f";

check_md5_output($md5, $cmd);
