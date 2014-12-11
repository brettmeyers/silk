#! /usr/bin/perl -w
# MD5: 9e65d0d47cfa84fb6d81ed1f05627e4b
# TEST: ./rwcut --fields=dip,dport,sip,sport --delimited --tail-recs=1000 --num-recs=2000 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=dip,dport,sip,sport --delimited --tail-recs=1000 --num-recs=2000 $file{data}";
my $md5 = "9e65d0d47cfa84fb6d81ed1f05627e4b";

check_md5_output($md5, $cmd);
