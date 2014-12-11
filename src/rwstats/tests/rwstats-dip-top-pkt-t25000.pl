#! /usr/bin/perl -w
# MD5: b0917e13da4ea4396498f1fc69d7945d
# TEST: ./rwstats --fields=dip --values=packets --threshold=25000 --top --ipv6-policy=ignore ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=dip --values=packets --threshold=25000 --top --ipv6-policy=ignore $file{data}";
my $md5 = "b0917e13da4ea4396498f1fc69d7945d";

check_md5_output($md5, $cmd);
