#! /usr/bin/perl -w
# MD5: bbea0c7b537dcf61e372674f4c57ea79
# TEST: ./rwtotal --sport --max-packet=20 --skip-zero ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --sport --max-packet=20 --skip-zero $file{data}";
my $md5 = "bbea0c7b537dcf61e372674f4c57ea79";

check_md5_output($md5, $cmd);
