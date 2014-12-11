#! /usr/bin/perl -w
# MD5: 5255512c759fb7dffe1baa7cb7e360f7
# TEST: ./rwbag --sport-packets=stdout ../../tests/data.rwf | ./rwbagcat --key-format=decimal --column-separator=, --no-final-delim

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sport-packets=stdout $file{data} | $rwbagcat --key-format=decimal --column-separator=, --no-final-delim";
my $md5 = "5255512c759fb7dffe1baa7cb7e360f7";

check_md5_output($md5, $cmd);
