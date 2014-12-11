#! /usr/bin/perl -w
# MD5: 188c2c1f278b498e80a2b00cc24bf19b
# TEST: ./rwtotal --packets --skip-zero ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --packets --skip-zero $file{data}";
my $md5 = "188c2c1f278b498e80a2b00cc24bf19b";

check_md5_output($md5, $cmd);
