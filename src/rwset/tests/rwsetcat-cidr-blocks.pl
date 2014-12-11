#! /usr/bin/perl -w
# MD5: cd161b9ab65ec1a57a417657f2d1d9f5
# TEST: ./rwset --sip-file=stdout ../../tests/data.rwf | ./rwsetcat --cidr-blocks

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my $rwset = check_silk_app('rwset');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwset --sip-file=stdout $file{data} | $rwsetcat --cidr-blocks";
my $md5 = "cd161b9ab65ec1a57a417657f2d1d9f5";

check_md5_output($md5, $cmd);
