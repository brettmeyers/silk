#! /usr/bin/perl -w
# MD5: c2d472bc3c9eb382a6aefb88456ce4b4
# TEST: ./rwstats --fields=dip --values=records --percentage=4 --ipv6-policy=ignore ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=dip --values=records --percentage=4 --ipv6-policy=ignore $file{data}";
my $md5 = "c2d472bc3c9eb382a6aefb88456ce4b4";

check_md5_output($md5, $cmd);
