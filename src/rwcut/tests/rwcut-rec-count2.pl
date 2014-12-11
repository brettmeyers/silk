#! /usr/bin/perl -w
# MD5: 40698b45c527d603b85383d9162d70c2
# TEST: ./rwcut --fields=9,10 --timestamp-format=epoch --num-recs=3000 --start-rec-num=2000 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=9,10 --timestamp-format=epoch --num-recs=3000 --start-rec-num=2000 $file{data}";
my $md5 = "40698b45c527d603b85383d9162d70c2";

check_md5_output($md5, $cmd);
