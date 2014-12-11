#! /usr/bin/perl -w
# MD5: 801e3a96fd06d45f22ed09e2d5fa871f
# TEST: ./rwstats --fields=dip --count=10 --top --ipv6-policy=ignore --presorted-input ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwstats --fields=dip --count=10 --top --ipv6-policy=ignore --presorted-input $file{empty}";
my $md5 = "801e3a96fd06d45f22ed09e2d5fa871f";

check_md5_output($md5, $cmd);
