#! /usr/bin/perl -w
# MD5: d41d8cd98f00b204e9800998ecf8427e
# TEST: ./rwset --sip-file=stdout ../../tests/empty.rwf | ./rwsetcat

use strict;
use SiLKTests;

my $rwset = check_silk_app('rwset');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwset --sip-file=stdout $file{empty} | $rwsetcat";
my $md5 = "d41d8cd98f00b204e9800998ecf8427e";

check_md5_output($md5, $cmd);
