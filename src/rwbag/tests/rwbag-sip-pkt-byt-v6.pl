#! /usr/bin/perl -w
# MD5: e5961c3d9e585cf7addfb98caf6ff50d
# TEST: ./rwbag --sip-packets=stdout --sip-bytes=/dev/null ../../tests/data-v6.rwf | ./rwbagcat

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwbag --sip-packets=stdout --sip-bytes=/dev/null $file{v6data} | $rwbagcat";
my $md5 = "e5961c3d9e585cf7addfb98caf6ff50d";

check_md5_output($md5, $cmd);
