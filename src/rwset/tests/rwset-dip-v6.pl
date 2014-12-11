#! /usr/bin/perl -w
# MD5: 9695f05722891ba9d536891fcece82e3
# TEST: ./rwset --dip-file=stdout ../../tests/data-v6.rwf | ./rwsetcat --cidr-blocks=0

use strict;
use SiLKTests;

my $rwset = check_silk_app('rwset');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipset_v6));
my $cmd = "$rwset --dip-file=stdout $file{v6data} | $rwsetcat --cidr-blocks=0";
my $md5 = "9695f05722891ba9d536891fcece82e3";

check_md5_output($md5, $cmd);
