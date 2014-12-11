#! /usr/bin/perl -w
# MD5: 39d130cda84b967416eeb95cc9f309b1
# TEST: ./rwsetcat --ip-ranges --print-filename=1 ../../tests/set1-v4.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
my $cmd = "$rwsetcat --ip-ranges --print-filename=1 $file{v4set1}";
my $md5 = "39d130cda84b967416eeb95cc9f309b1";

check_md5_output($md5, $cmd);
