#! /usr/bin/perl -w
# MD5: 3677d3da40803d98298314b69fadf06a
# TEST: ./rwset --sip-file=stdout ../../tests/data.rwf | ./rwsetcat --cidr-blocks | ./rwsetbuild | ./rwsetcat

use strict;
use SiLKTests;

my $rwsetbuild = check_silk_app('rwsetbuild');
my $rwset = check_silk_app('rwset');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwset --sip-file=stdout $file{data} | $rwsetcat --cidr-blocks | $rwsetbuild | $rwsetcat";
my $md5 = "3677d3da40803d98298314b69fadf06a";

check_md5_output($md5, $cmd);
