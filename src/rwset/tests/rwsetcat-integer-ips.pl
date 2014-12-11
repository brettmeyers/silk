#! /usr/bin/perl -w
# MD5: b72944d8321adb82a87628a9a7966712
# TEST: ./rwset --sip-file=stdout ../../tests/data.rwf | ./rwsetcat --ip-format=decimal

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my $rwset = check_silk_app('rwset');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwset --sip-file=stdout $file{data} | $rwsetcat --ip-format=decimal";
my $md5 = "b72944d8321adb82a87628a9a7966712";

check_md5_output($md5, $cmd);
