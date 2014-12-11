#! /usr/bin/perl -w
# MD5: 42cef866f843d525b0a1bd509213808a
# TEST: ../rwfilter/rwfilter --proto=1 --pass=- ../../tests/data.rwf | ./rwcut --fields=4,5 --icmp-type-and-code

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --proto=1 --pass=- $file{data} | $rwcut --fields=4,5 --icmp-type-and-code";
my $md5 = "42cef866f843d525b0a1bd509213808a";

check_md5_output($md5, $cmd);
