#! /usr/bin/perl -w
# MD5: 002cf9d890490833ebfd5ecb6d7a8206
# TEST: ./rwtotal --sport --min-packet=20 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --sport --min-packet=20 $file{data}";
my $md5 = "002cf9d890490833ebfd5ecb6d7a8206";

check_md5_output($md5, $cmd);
