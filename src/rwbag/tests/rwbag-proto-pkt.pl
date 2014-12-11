#! /usr/bin/perl -w
# MD5: 295fce4fc7d7579691ac9ba61c899b7a
# TEST: ./rwbag --proto-packets=stdout ../../tests/data.rwf | ./rwbagcat --key-format=decimal --maxkey=17

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --proto-packets=stdout $file{data} | $rwbagcat --key-format=decimal --maxkey=17";
my $md5 = "295fce4fc7d7579691ac9ba61c899b7a";

check_md5_output($md5, $cmd);
