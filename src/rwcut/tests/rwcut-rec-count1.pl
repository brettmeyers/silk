#! /usr/bin/perl -w
# MD5: 69842a94036246ceb9bed57fb93a13a7
# TEST: ./rwcut --fields=3-5 --num-recs=3000 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=3-5 --num-recs=3000 $file{data}";
my $md5 = "69842a94036246ceb9bed57fb93a13a7";

check_md5_output($md5, $cmd);
