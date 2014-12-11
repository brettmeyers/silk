#! /usr/bin/perl -w
# MD5: 3677d3da40803d98298314b69fadf06a
# TEST: ./rwset --sip-file=stdout ../../tests/empty.rwf ../../tests/data.rwf ../../tests/empty.rwf | ./rwsetcat --cidr-blocks=0

use strict;
use SiLKTests;

my $rwset = check_silk_app('rwset');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{data} = get_data_or_exit77('data');
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwset --sip-file=stdout $file{empty} $file{data} $file{empty} | $rwsetcat --cidr-blocks=0";
my $md5 = "3677d3da40803d98298314b69fadf06a";

check_md5_output($md5, $cmd);
