#! /usr/bin/perl -w
# MD5: de27127e893271d9ff0d30fc7a97182e
# TEST: ./rwbag --dport-flow=stdout ../../tests/data.rwf | ./rwbagcat --key-format=decimal --delimited

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --dport-flow=stdout $file{data} | $rwbagcat --key-format=decimal --delimited";
my $md5 = "de27127e893271d9ff0d30fc7a97182e";

check_md5_output($md5, $cmd);
