#! /usr/bin/perl -w
# MD5: fc0690bbd0932b2d445222f074f02d15
# TEST: ./rwfilter --proto=17 --max-pass=100 --max-fail=200 --pass=/tmp/rwfilter-max-pass-fail-pass --fail=/tmp/rwfilter-max-pass-fail-fail ../../tests/data.rwf && ../rwcut/rwcut --fields=1-10 --ipv6-policy=ignore /tmp/rwfilter-max-pass-fail-pass /tmp/rwfilter-max-pass-fail-fail

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my %temp;
$temp{pass} = make_tempname('pass');
$temp{fail} = make_tempname('fail');
my $cmd = "$rwfilter --proto=17 --max-pass=100 --max-fail=200 --pass=$temp{pass} --fail=$temp{fail} $file{data} && $rwcut --fields=1-10 --ipv6-policy=ignore $temp{pass} $temp{fail}";
my $md5 = "fc0690bbd0932b2d445222f074f02d15";

check_md5_output($md5, $cmd);
