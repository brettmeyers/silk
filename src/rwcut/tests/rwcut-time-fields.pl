#! /usr/bin/perl -w
# MD5: 68f86f5e39a8f9ff12ea0f610b403285
# TEST: ./rwcut --fields=9-11 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=9-11 $file{data}";
my $md5 = "68f86f5e39a8f9ff12ea0f610b403285";

check_md5_output($md5, $cmd);
