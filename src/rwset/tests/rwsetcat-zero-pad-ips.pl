#! /usr/bin/perl -w
# MD5: dffef1c1c722a4b975362e9475b9b262
# TEST: ./rwset --sip-file=stdout ../../tests/data.rwf | ./rwsetcat --ip-format=zero-padded stdin

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my $rwset = check_silk_app('rwset');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwset --sip-file=stdout $file{data} | $rwsetcat --ip-format=zero-padded stdin";
my $md5 = "dffef1c1c722a4b975362e9475b9b262";

check_md5_output($md5, $cmd);
