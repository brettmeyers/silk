#! /usr/bin/perl -w
# MD5: 42225c369e5eab1756000cb6582cf354
# TEST: ./rwcount --bin-size=3600 --load-scheme=1 --timestamp-format=m/d/y ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=3600 --load-scheme=1 --timestamp-format=m/d/y $file{data}";
my $md5 = "42225c369e5eab1756000cb6582cf354";

check_md5_output($md5, $cmd);
