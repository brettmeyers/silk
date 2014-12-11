#! /usr/bin/perl -w
# MD5: 1262d14f8982acea841db26629b74b2a
# TEST: ./rwstats --fields=protocol --values=packets --count=15 --bottom ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=protocol --values=packets --count=15 --bottom $file{data}";
my $md5 = "1262d14f8982acea841db26629b74b2a";

check_md5_output($md5, $cmd);
