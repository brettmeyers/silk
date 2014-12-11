#! /usr/bin/perl -w
# MD5: f6bdf2e5bb183525d0e938e29204afe9
# TEST: ./rwbag --sip-flows=stdout ../../tests/data.rwf | ./rwbagcat --network-structure

use strict;
use SiLKTests;

my $rwbagcat = check_silk_app('rwbagcat');
my $rwbag = check_silk_app('rwbag');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sip-flows=stdout $file{data} | $rwbagcat --network-structure";
my $md5 = "f6bdf2e5bb183525d0e938e29204afe9";

check_md5_output($md5, $cmd);
