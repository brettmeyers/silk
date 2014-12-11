#! /usr/bin/perl -w
# MD5: 0807bf894a773d82c86377ed9046f254
# TEST: ./rwsetcat --count-ips ../../tests/set2-v4.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsetcat --count-ips $file{v4set2}";
my $md5 = "0807bf894a773d82c86377ed9046f254";

check_md5_output($md5, $cmd);
