#! /usr/bin/perl -w
# MD5: 239e7c84cbd99dcd8d5db0ff08e9be90
# TEST: ./rwtotal --dport ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --dport $file{data}";
my $md5 = "239e7c84cbd99dcd8d5db0ff08e9be90";

check_md5_output($md5, $cmd);
