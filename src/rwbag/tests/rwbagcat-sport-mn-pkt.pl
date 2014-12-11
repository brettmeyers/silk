#! /usr/bin/perl -w
# MD5: 8dfaf9449901adc345d267d3224166e3
# TEST: ./rwbag --sport-packets=stdout ../../tests/data.rwf | ./rwbagcat --key-format=decimal --mincounter=20

use strict;
use SiLKTests;

my $rwbagcat = check_silk_app('rwbagcat');
my $rwbag = check_silk_app('rwbag');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sport-packets=stdout $file{data} | $rwbagcat --key-format=decimal --mincounter=20";
my $md5 = "8dfaf9449901adc345d267d3224166e3";

check_md5_output($md5, $cmd);
