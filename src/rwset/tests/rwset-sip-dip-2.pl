#! /usr/bin/perl -w
# MD5: f1592865c0b6354403a12d4746498f02
# TEST: ./rwset --sip=/dev/null --dip=stdout ../../tests/data.rwf | ./rwsetcat --cidr-blocks=0

use strict;
use SiLKTests;

my $rwset = check_silk_app('rwset');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwset --sip=/dev/null --dip=stdout $file{data} | $rwsetcat --cidr-blocks=0";
my $md5 = "f1592865c0b6354403a12d4746498f02";

check_md5_output($md5, $cmd);
