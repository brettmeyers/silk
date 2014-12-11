#! /usr/bin/perl -w
# MD5: 5a89125f8be393495f3ae2a4c53de63e
# TEST: ./rwset --sip-file=stdout ../../tests/data.rwf | ./rwsetcat --network-structure=12TS,12

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my $rwset = check_silk_app('rwset');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwset --sip-file=stdout $file{data} | $rwsetcat --network-structure=12TS,12";
my $md5 = "5a89125f8be393495f3ae2a4c53de63e";

check_md5_output($md5, $cmd);
