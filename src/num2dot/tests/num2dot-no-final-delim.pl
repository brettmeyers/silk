#! /usr/bin/perl -w
# MD5: 8b070e5f8d45e4eb9c549bf0f19c8891
# TEST: ../rwcut/rwcut --fields=1,2 --no-title --ipv6-policy=ignore --ip-format=decimal --no-final-delimiter ../../tests/data.rwf | ./num2dot --ip-fields=2,1

use strict;
use SiLKTests;

my $num2dot = check_silk_app('num2dot');
my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=1,2 --no-title --ipv6-policy=ignore --ip-format=decimal --no-final-delimiter $file{data} | $num2dot --ip-fields=2,1";
my $md5 = "8b070e5f8d45e4eb9c549bf0f19c8891";

check_md5_output($md5, $cmd);
