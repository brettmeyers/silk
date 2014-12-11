#! /usr/bin/perl -w
# MD5: 5edfd7530a1d2107e9b43d44b25ee839
# TEST: ./rwcut --fields=9 --timestamp-format=epoch --no-final-delimiter ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=9 --timestamp-format=epoch --no-final-delimiter $file{data}";
my $md5 = "5edfd7530a1d2107e9b43d44b25ee839";

check_md5_output($md5, $cmd);
