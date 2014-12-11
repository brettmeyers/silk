#! /usr/bin/perl -w
# MD5: cc7de82b6cf7fb305ef77eac634e3f84
# TEST: ./rwtotal --sport ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwtotal --sport $file{empty}";
my $md5 = "cc7de82b6cf7fb305ef77eac634e3f84";

check_md5_output($md5, $cmd);
