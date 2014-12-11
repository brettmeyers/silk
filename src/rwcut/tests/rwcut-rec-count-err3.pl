#! /usr/bin/perl -w
# ERR_MD5: ec003e4ba8aceb9f621106c6f417bde4
# TEST: ./rwcut --fields=sport,dport --end-rec-num=300 --tail-recs=100 ../../tests/data.rwf 2>&1

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=sport,dport --end-rec-num=300 --tail-recs=100 $file{data} 2>&1";
my $md5 = "ec003e4ba8aceb9f621106c6f417bde4";

check_md5_output($md5, $cmd, 1);
