#! /usr/bin/perl -w
# MD5: 1f8d46589f293b8ecc7119a4dbdfb71f
# TEST: ./rwstats --count=10 --fields=dip --column-sep=/ --top --ipv6-policy=ignore ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --count=10 --fields=dip --column-sep=/ --top --ipv6-policy=ignore $file{data}";
my $md5 = "1f8d46589f293b8ecc7119a4dbdfb71f";

check_md5_output($md5, $cmd);
