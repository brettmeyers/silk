#! /usr/bin/perl -w
# MD5: 121a9b7d2a25723bc50b50dff2dc66e4
# TEST: ./rwcut --fields=sip,dip --delimited=, --num-recs=3000 --end-rec-num=20000 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=sip,dip --delimited=, --num-recs=3000 --end-rec-num=20000 $file{data}";
my $md5 = "121a9b7d2a25723bc50b50dff2dc66e4";

check_md5_output($md5, $cmd);
