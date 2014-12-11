#! /usr/bin/perl -w
# MD5: 0d5778c870df50d3901e04324bf5dd2c
# TEST: ../rwcut/rwcut --fields=1,3,2,4,5 --no-title --ipv6-policy=ignore --ip-format=decimal ../../tests/data.rwf | ./num2dot --ip-fields=1,3

use strict;
use SiLKTests;

my $num2dot = check_silk_app('num2dot');
my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=1,3,2,4,5 --no-title --ipv6-policy=ignore --ip-format=decimal $file{data} | $num2dot --ip-fields=1,3";
my $md5 = "0d5778c870df50d3901e04324bf5dd2c";

check_md5_output($md5, $cmd);
