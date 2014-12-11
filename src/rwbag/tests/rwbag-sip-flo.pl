#! /usr/bin/perl -w
# MD5: bb127fe6e2e3fd68632ad38a0740eab8
# TEST: ./rwbag --sip-flows=stdout ../../tests/data.rwf | ./rwbagcat

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sip-flows=stdout $file{data} | $rwbagcat";
my $md5 = "bb127fe6e2e3fd68632ad38a0740eab8";

check_md5_output($md5, $cmd);
