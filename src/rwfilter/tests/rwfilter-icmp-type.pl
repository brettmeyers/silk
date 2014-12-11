#! /usr/bin/perl -w
# MD5: 7f368933743b7bb7fac8e1931dcb7031
# TEST: ./rwfilter --icmp-type=3 --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --icmp-type=3 --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "7f368933743b7bb7fac8e1931dcb7031";

check_md5_output($md5, $cmd);
