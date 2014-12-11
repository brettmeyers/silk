#! /usr/bin/perl -w
# MD5: 9790c86ecbb95868212e1756b230d256
# TEST: ./rwbag --sport-packets=stdout ../../tests/data.rwf | ./rwbagcat --key-format=decimal --maxcounter=20

use strict;
use SiLKTests;

my $rwbagcat = check_silk_app('rwbagcat');
my $rwbag = check_silk_app('rwbag');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sport-packets=stdout $file{data} | $rwbagcat --key-format=decimal --maxcounter=20";
my $md5 = "9790c86ecbb95868212e1756b230d256";

check_md5_output($md5, $cmd);
