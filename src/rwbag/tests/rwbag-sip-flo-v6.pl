#! /usr/bin/perl -w
# MD5: ca78df5022e6feeef8a997203991e3c5
# TEST: ./rwbag --sip-flows=stdout ../../tests/data-v6.rwf | ./rwbagcat

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwbag --sip-flows=stdout $file{v6data} | $rwbagcat";
my $md5 = "ca78df5022e6feeef8a997203991e3c5";

check_md5_output($md5, $cmd);
