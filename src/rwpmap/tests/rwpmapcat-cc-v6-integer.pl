#! /usr/bin/perl -w
# MD5: 2b9131d869e8da9edae05da435e29f24
# TEST: ./rwpmapcat --no-cidr ../../tests/fake-cc-v6.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{v6_fake_cc} = get_data_or_exit77('v6_fake_cc');
check_features(qw(ipv6));
my $cmd = "$rwpmapcat --no-cidr $file{v6_fake_cc}";
my $md5 = "2b9131d869e8da9edae05da435e29f24";

check_md5_output($md5, $cmd);
