#! /usr/bin/perl -w
# MD5: f29b575ac15116a0d5e5d19e625395ba
# TEST: ./rwtotal --sport --no-titles ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --sport --no-titles $file{data}";
my $md5 = "f29b575ac15116a0d5e5d19e625395ba";

check_md5_output($md5, $cmd);
