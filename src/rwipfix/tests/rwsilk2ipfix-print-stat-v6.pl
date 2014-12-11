#! /usr/bin/perl -w
# MD5: a217878314c95cca48b24ee6682ea7c5
# TEST: ./rwsilk2ipfix ../../tests/data-v6.rwf --ipfix-output=/dev/null --print-stat 2>&1

use strict;
use SiLKTests;

my $rwsilk2ipfix = check_silk_app('rwsilk2ipfix');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipfix ipv6));
my $cmd = "$rwsilk2ipfix $file{v6data} --ipfix-output=/dev/null --print-stat 2>&1";
my $md5 = "a217878314c95cca48b24ee6682ea7c5";

check_md5_output($md5, $cmd);
