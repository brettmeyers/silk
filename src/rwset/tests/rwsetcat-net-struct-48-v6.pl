#! /usr/bin/perl -w
# MD5: 0ec9fc8b60b20ce9d8c0768f4e34dad6
# TEST: ./rwset --sip-file=stdout ../../tests/data-v6.rwf | ./rwsetcat --network-structure=v6:48,T/48,64,123,112

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my $rwset = check_silk_app('rwset');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipset_v6));
my $cmd = "$rwset --sip-file=stdout $file{v6data} | $rwsetcat --network-structure=v6:48,T/48,64,123,112";
my $md5 = "0ec9fc8b60b20ce9d8c0768f4e34dad6";

check_md5_output($md5, $cmd);
