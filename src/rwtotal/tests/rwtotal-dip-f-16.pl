#! /usr/bin/perl -w
# MD5: c7240ee13744d9a468b1108fbdb40c8a
# TEST: ./rwtotal --dip-first-16 --skip-zero ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --dip-first-16 --skip-zero $file{data}";
my $md5 = "c7240ee13744d9a468b1108fbdb40c8a";

check_md5_output($md5, $cmd);
