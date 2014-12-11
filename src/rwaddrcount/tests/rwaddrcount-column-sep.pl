#! /usr/bin/perl -w
# MD5: 547f21a5e2d2aa413336ebd12d56fb73
# TEST: ./rwaddrcount --print-rec --sort-ips --column-separator=/ --no-final-delimiter ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwaddrcount --print-rec --sort-ips --column-separator=/ --no-final-delimiter $file{data}";
my $md5 = "547f21a5e2d2aa413336ebd12d56fb73";

check_md5_output($md5, $cmd);
