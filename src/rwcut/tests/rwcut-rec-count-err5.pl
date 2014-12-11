#! /usr/bin/perl -w
# ERR_MD5: 46f7889d2bb77f1f0d04ebb4b7f37b2e
# TEST: ./rwcut --fields=sport,dport --start-rec-num=0 ../../tests/data.rwf 2>&1

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=sport,dport --start-rec-num=0 $file{data} 2>&1";
my $md5 = "46f7889d2bb77f1f0d04ebb4b7f37b2e";

check_md5_output($md5, $cmd, 1);
