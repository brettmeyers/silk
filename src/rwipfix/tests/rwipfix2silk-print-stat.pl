#! /usr/bin/perl -w
# MD5: 3f341172cc3cb3b55f35fe27641c27d8
# TEST: ./rwsilk2ipfix ../../tests/data.rwf | ./rwipfix2silk --silk-output=/dev/null --log-destination=stderr --print-stat 2>&1

use strict;
use SiLKTests;

my $rwipfix2silk = check_silk_app('rwipfix2silk');
my $rwsilk2ipfix = check_silk_app('rwsilk2ipfix');
my %file;
$file{data} = get_data_or_exit77('data');
check_features(qw(ipfix));
my $cmd = "$rwsilk2ipfix $file{data} | $rwipfix2silk --silk-output=/dev/null --log-destination=stderr --print-stat 2>&1";
my $md5 = "3f341172cc3cb3b55f35fe27641c27d8";

check_md5_output($md5, $cmd);
