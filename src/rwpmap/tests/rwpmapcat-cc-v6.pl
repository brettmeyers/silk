#! /usr/bin/perl -w
# MD5: 2f09e7a8c4d1a42aef3a5420bfd12a84
# TEST: ./rwpmapcat --no-cidr --country-codes=../../tests/fake-cc-v6.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{v6_fake_cc} = get_data_or_exit77('v6_fake_cc');
check_features(qw(ipv6));
my $cmd = "$rwpmapcat --no-cidr --country-codes=$file{v6_fake_cc}";
my $md5 = "2f09e7a8c4d1a42aef3a5420bfd12a84";

check_md5_output($md5, $cmd);
