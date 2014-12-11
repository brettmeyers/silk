#! /usr/bin/perl -w
# MD5: 919dcedf3ac8a7f374509dd5e8ee8df3
# TEST: ./rwbag --sip-flows=stdout ../../tests/data.rwf | ./rwbagcat --network-structure=12TS,12

use strict;
use SiLKTests;

my $rwbagcat = check_silk_app('rwbagcat');
my $rwbag = check_silk_app('rwbag');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sip-flows=stdout $file{data} | $rwbagcat --network-structure=12TS,12";
my $md5 = "919dcedf3ac8a7f374509dd5e8ee8df3";

check_md5_output($md5, $cmd);
