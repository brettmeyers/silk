#! /usr/bin/perl -w
# MD5: f68bc6af3497081cc592d516f236f920
# TEST: ./rwtotal --sport --no-column --column-sep=, ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --sport --no-column --column-sep=, $file{data}";
my $md5 = "f68bc6af3497081cc592d516f236f920";

check_md5_output($md5, $cmd);
