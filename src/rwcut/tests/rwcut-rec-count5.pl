#! /usr/bin/perl -w
# MD5: 36e14c413f15afb9e5002e4a9d37a4f6
# TEST: ./rwcut --fields=sport,dport --start-rec-num=30000 --end-rec-num=40000 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=sport,dport --start-rec-num=30000 --end-rec-num=40000 $file{data}";
my $md5 = "36e14c413f15afb9e5002e4a9d37a4f6";

check_md5_output($md5, $cmd);
