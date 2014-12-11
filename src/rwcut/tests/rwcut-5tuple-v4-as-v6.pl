#! /usr/bin/perl -w
# MD5: ac634afb65d11a67a6fe0ac7f594aa11
# TEST: ./rwcut --fields=1-5 --ipv6-policy=force ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
check_features(qw(ipv6));
my $cmd = "$rwcut --fields=1-5 --ipv6-policy=force $file{data}";
my $md5 = "ac634afb65d11a67a6fe0ac7f594aa11";

check_md5_output($md5, $cmd);
