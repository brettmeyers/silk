#! /usr/bin/perl -w
# MD5: f869dfd34aaeeb41a8dc60e3550f0bae
# TEST: ./rwaddrcount --use-dest --print-stat ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwaddrcount --use-dest --print-stat $file{data}";
my $md5 = "f869dfd34aaeeb41a8dc60e3550f0bae";

check_md5_output($md5, $cmd);
