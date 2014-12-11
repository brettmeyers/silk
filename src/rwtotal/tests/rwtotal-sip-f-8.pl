#! /usr/bin/perl -w
# MD5: e83a934d5bc18d787de0129daabb142c
# TEST: ./rwtotal --sip-first-8 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --sip-first-8 $file{data}";
my $md5 = "e83a934d5bc18d787de0129daabb142c";

check_md5_output($md5, $cmd);
