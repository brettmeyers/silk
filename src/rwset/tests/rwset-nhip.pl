#! /usr/bin/perl -w
# MD5: dea8a6f0b1d10624b3c1af6e26f56f1b
# TEST: ./rwset --nhip-file=stdout ../../tests/data.rwf | ./rwsetcat

use strict;
use SiLKTests;

my $rwset = check_silk_app('rwset');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwset --nhip-file=stdout $file{data} | $rwsetcat";
my $md5 = "dea8a6f0b1d10624b3c1af6e26f56f1b";

check_md5_output($md5, $cmd);
