#! /usr/bin/perl -w
# MD5: d728767cef4faed65489098a5cfd4075
# TEST: ./rwsetcat --network-structure=20TS,20 ../../tests/set2-v4.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsetcat --network-structure=20TS,20 $file{v4set2}";
my $md5 = "d728767cef4faed65489098a5cfd4075";

check_md5_output($md5, $cmd);
