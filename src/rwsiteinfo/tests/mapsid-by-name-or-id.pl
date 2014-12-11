#! /usr/bin/perl -w
# MD5: b62e7d7b1356c3e58a9c8a51b31dbf38
# TEST: ./mapsid S9 8 S11 10 S7

use strict;
use SiLKTests;

my $mapsid = check_silk_app('mapsid');
my $cmd = "$mapsid S9 8 S11 10 S7";
my $md5 = "b62e7d7b1356c3e58a9c8a51b31dbf38";

check_md5_output($md5, $cmd);
