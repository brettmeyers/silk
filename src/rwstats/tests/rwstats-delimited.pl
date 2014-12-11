#! /usr/bin/perl -w
# MD5: c1e5681c6de19c5784e97be4ce2ba740
# TEST: ./rwstats --fields=dip --count=10 --delimited=, --top --ipv6-policy=ignore ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=dip --count=10 --delimited=, --top --ipv6-policy=ignore $file{data}";
my $md5 = "c1e5681c6de19c5784e97be4ce2ba740";

check_md5_output($md5, $cmd);
