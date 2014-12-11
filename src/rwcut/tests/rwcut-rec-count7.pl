#! /usr/bin/perl -w
# MD5: cb4e94ac7b8e47b9e65f70009ad31115
# TEST: ./rwcut --fields=class,type,sensor --tail-recs=2000 --num-recs=1000 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=class,type,sensor --tail-recs=2000 --num-recs=1000 $file{data}";
my $md5 = "cb4e94ac7b8e47b9e65f70009ad31115";

check_md5_output($md5, $cmd);
