#! /usr/bin/perl -w
# MD5: 08a3cf23bcd728d79f8ca9bd9d43e693
# TEST: ./rwbag --sip-flows=stdout ../../tests/data-v6.rwf | ./rwbagcat --key-format=decimal --bin-ips

use strict;
use SiLKTests;

my $rwbagcat = check_silk_app('rwbagcat');
my $rwbag = check_silk_app('rwbag');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwbag --sip-flows=stdout $file{v6data} | $rwbagcat --key-format=decimal --bin-ips";
my $md5 = "08a3cf23bcd728d79f8ca9bd9d43e693";

check_md5_output($md5, $cmd);
