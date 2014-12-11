#! /usr/bin/perl -w
# MD5: e769ea9d8ab652bf73eb77254d4a4ce1
# TEST: ./rwtotal --sip-first-24 --skip-zero ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --sip-first-24 --skip-zero $file{data}";
my $md5 = "e769ea9d8ab652bf73eb77254d4a4ce1";

check_md5_output($md5, $cmd);
