#! /usr/bin/perl -w
# MD5: 31f402720bffca63e8dbc9ce239c9fec
# TEST: ./rwbag --sport-bytes=stdout ../../tests/data.rwf | ./rwbagcat --key-format=decimal --maxcounter=2000

use strict;
use SiLKTests;

my $rwbagcat = check_silk_app('rwbagcat');
my $rwbag = check_silk_app('rwbag');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sport-bytes=stdout $file{data} | $rwbagcat --key-format=decimal --maxcounter=2000";
my $md5 = "31f402720bffca63e8dbc9ce239c9fec";

check_md5_output($md5, $cmd);
