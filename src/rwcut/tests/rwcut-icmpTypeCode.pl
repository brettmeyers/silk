#! /usr/bin/perl -w
# MD5: d3f4477849628438a36239e7519fdc7e
# TEST: ../rwfilter/rwfilter --proto=1 --pass=- ../../tests/data.rwf | ./rwcut --fields=icmpTypeCode

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --proto=1 --pass=- $file{data} | $rwcut --fields=icmpTypeCode";
my $md5 = "d3f4477849628438a36239e7519fdc7e";

check_md5_output($md5, $cmd);
