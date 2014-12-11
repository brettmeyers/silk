#! /usr/bin/perl -w
# MD5: 63287d55706ec1a07641caf92db42cb8
# TEST: ./rwbag --sip-bytes=stdout ../../tests/data-v6.rwf | ./rwbagcat

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwbag --sip-bytes=stdout $file{v6data} | $rwbagcat";
my $md5 = "63287d55706ec1a07641caf92db42cb8";

check_md5_output($md5, $cmd);
