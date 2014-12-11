#! /usr/bin/perl -w
# MD5: 6872aaa92c839d30fcf36eb96dd4c2e0
# TEST: ../rwfilter/rwfilter --proto=1 --pass=- ../../tests/data.rwf | ./rwtotal --icmp-code

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --proto=1 --pass=- $file{data} | $rwtotal --icmp-code";
my $md5 = "6872aaa92c839d30fcf36eb96dd4c2e0";

check_md5_output($md5, $cmd);
