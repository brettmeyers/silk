#! /usr/bin/perl -w
# MD5: 54d6d129f9bdeedc7a1f0014954d5af3
# TEST: ./rwtotal --sip-last-8 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --sip-last-8 $file{data}";
my $md5 = "54d6d129f9bdeedc7a1f0014954d5af3";

check_md5_output($md5, $cmd);
