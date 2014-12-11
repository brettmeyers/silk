#! /usr/bin/perl -w
# MD5: 40e1d81bad3142ed24a7af7debcd142a
# TEST: ./rwcut --fields=12 --integer-sensor --num-recs=3000 --end-rec-num=2000 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=12 --integer-sensor --num-recs=3000 --end-rec-num=2000 $file{data}";
my $md5 = "40e1d81bad3142ed24a7af7debcd142a";

check_md5_output($md5, $cmd);
