#! /usr/bin/perl -w
# MD5: a41ae1525d10e90d49711486e98c159e
# TEST: ./rwset --sip-file=stdout ../../tests/data.rwf | ./rwsetcat --count-ips

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my $rwset = check_silk_app('rwset');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwset --sip-file=stdout $file{data} | $rwsetcat --count-ips";
my $md5 = "a41ae1525d10e90d49711486e98c159e";

check_md5_output($md5, $cmd);
