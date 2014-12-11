#! /usr/bin/perl -w
# MD5: 881b7cd5a2af2884887431d99ece3205
# TEST: ./rwbag --sip-flows=stdout ../../tests/data-v6.rwf | ./rwbagtool --invert | ./rwbagcat --key-format=decimal

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwbag --sip-flows=stdout $file{v6data} | $rwbagtool --invert | $rwbagcat --key-format=decimal";
my $md5 = "881b7cd5a2af2884887431d99ece3205";

check_md5_output($md5, $cmd);
