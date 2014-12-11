#! /usr/bin/perl -w
# ERR_MD5: 65c364dd1f7a2a8b6b6dac9f2d741c8f
# TEST: ./rwcut --fields=sport,dport --tail-recs=0 ../../tests/data.rwf 2>&1

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=sport,dport --tail-recs=0 $file{data} 2>&1";
my $md5 = "65c364dd1f7a2a8b6b6dac9f2d741c8f";

check_md5_output($md5, $cmd, 1);
