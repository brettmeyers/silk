#! /usr/bin/perl -w
# MD5: e210044fb92f3a183e4634477b4c5f9a
# TEST: ./rwset --sip=/dev/null --dip=stdout ../../tests/data-v6.rwf | ./rwsetcat --cidr-blocks=0 --ip-format=decimal

use strict;
use SiLKTests;

my $rwset = check_silk_app('rwset');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipset_v6));
my $cmd = "$rwset --sip=/dev/null --dip=stdout $file{v6data} | $rwsetcat --cidr-blocks=0 --ip-format=decimal";
my $md5 = "e210044fb92f3a183e4634477b4c5f9a";

check_md5_output($md5, $cmd);
