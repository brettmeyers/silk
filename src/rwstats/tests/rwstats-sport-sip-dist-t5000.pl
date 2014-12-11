#! /usr/bin/perl -w
# MD5: d90b55c97b3012b792702a849f15ee0e
# TEST: ./rwstats --fields=sport --values=sip-distinct --threshold=5000 --no-percent ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=sport --values=sip-distinct --threshold=5000 --no-percent $file{data}";
my $md5 = "d90b55c97b3012b792702a849f15ee0e";

check_md5_output($md5, $cmd);
