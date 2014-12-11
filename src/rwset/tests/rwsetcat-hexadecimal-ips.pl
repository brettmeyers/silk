#! /usr/bin/perl -w
# MD5: f39eb9047d97c1c6d5ded7be5cf88f38
# TEST: ./rwset --sip-file=stdout ../../tests/data.rwf | ./rwsetcat --ip-format=hexadecimal stdin

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my $rwset = check_silk_app('rwset');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwset --sip-file=stdout $file{data} | $rwsetcat --ip-format=hexadecimal stdin";
my $md5 = "f39eb9047d97c1c6d5ded7be5cf88f38";

check_md5_output($md5, $cmd);
