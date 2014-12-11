#! /usr/bin/perl -w
# MD5: 776ff75843bd817148ca277f1133fdeb
# TEST: ./rwsetcat --cidr ../../tests/set2-v4.set | ./rwsetbuild | ./rwsetcat --cidr

use strict;
use SiLKTests;

my $rwsetbuild = check_silk_app('rwsetbuild');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsetcat --cidr $file{v4set2} | $rwsetbuild | $rwsetcat --cidr";
my $md5 = "776ff75843bd817148ca277f1133fdeb";

check_md5_output($md5, $cmd);
