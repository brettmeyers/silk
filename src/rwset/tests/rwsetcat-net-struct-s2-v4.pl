#! /usr/bin/perl -w
# MD5: 0f453065b675061d07598bbaeff6ecbd
# TEST: ./rwsetcat --network-structure ../../tests/set2-v4.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsetcat --network-structure $file{v4set2}";
my $md5 = "0f453065b675061d07598bbaeff6ecbd";

check_md5_output($md5, $cmd);
