#! /usr/bin/perl -w
# MD5: 776ff75843bd817148ca277f1133fdeb
# TEST: ./rwsetcat --cidr-blocks ../../tests/set2-v4.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsetcat --cidr-blocks $file{v4set2}";
my $md5 = "776ff75843bd817148ca277f1133fdeb";

check_md5_output($md5, $cmd);
