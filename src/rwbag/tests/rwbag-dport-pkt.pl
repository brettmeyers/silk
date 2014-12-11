#! /usr/bin/perl -w
# MD5: 5bf70ac6a1af5fb734ed6cd75497c3bb
# TEST: ./rwbag --dport-packets=stdout ../../tests/data.rwf | ./rwbagcat --key-format=decimal --no-columns

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --dport-packets=stdout $file{data} | $rwbagcat --key-format=decimal --no-columns";
my $md5 = "5bf70ac6a1af5fb734ed6cd75497c3bb";

check_md5_output($md5, $cmd);
