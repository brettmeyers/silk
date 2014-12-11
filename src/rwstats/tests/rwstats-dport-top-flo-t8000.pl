#! /usr/bin/perl -w
# MD5: 6d4f0df0a5160f5f41a0d7fb68b901db
# TEST: ./rwstats --fields=dport --threshold=8000 --top ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=dport --threshold=8000 --top $file{data}";
my $md5 = "6d4f0df0a5160f5f41a0d7fb68b901db";

check_md5_output($md5, $cmd);
