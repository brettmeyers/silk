#! /usr/bin/perl -w
# ERR_MD5: 91f21c186218df6a67f2fb5cb94ea52b
# TEST: ./rwcut --fields=sport,dport --start-rec-num=300 --end-rec-num=100 ../../tests/data.rwf 2>&1

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=sport,dport --start-rec-num=300 --end-rec-num=100 $file{data} 2>&1";
my $md5 = "91f21c186218df6a67f2fb5cb94ea52b";

check_md5_output($md5, $cmd, 1);
