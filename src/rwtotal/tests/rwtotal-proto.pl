#! /usr/bin/perl -w
# MD5: bdcf18a44aa9ab9704432e0c39cc240c
# TEST: ./rwtotal --proto ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --proto $file{data}";
my $md5 = "bdcf18a44aa9ab9704432e0c39cc240c";

check_md5_output($md5, $cmd);
