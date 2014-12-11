#! /usr/bin/perl -w
# MD5: 3677d3da40803d98298314b69fadf06a
# TEST: ./rwset --sip-file=/dev/null --copy-input=stdout ../../tests/data.rwf | ./rwset --sip-file=- | ./rwsetcat --print-ips

use strict;
use SiLKTests;

my $rwset = check_silk_app('rwset');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwset --sip-file=/dev/null --copy-input=stdout $file{data} | $rwset --sip-file=- | $rwsetcat --print-ips";
my $md5 = "3677d3da40803d98298314b69fadf06a";

check_md5_output($md5, $cmd);
