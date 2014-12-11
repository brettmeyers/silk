#! /usr/bin/perl -w
# MD5: 08bb6cd60716b0fc916c6f2c019aa300
# TEST: ./rwbag --dip-bytes=stdout ../../tests/data-v6.rwf | ./rwbagcat

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwbag --dip-bytes=stdout $file{v6data} | $rwbagcat";
my $md5 = "08bb6cd60716b0fc916c6f2c019aa300";

check_md5_output($md5, $cmd);
