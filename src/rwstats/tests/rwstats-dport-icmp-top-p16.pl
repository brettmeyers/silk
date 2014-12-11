#! /usr/bin/perl -w
# MD5: 50f5f16e92e627def9f7d5f9a6e58b7b
# TEST: ../rwfilter/rwfilter --dport=68 --fail=- ../../tests/data.rwf | ./rwstats --fields=proto,dport,iType,iCode --count=16

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --dport=68 --fail=- $file{data} | ./rwstats --fields=proto,dport,iType,iCode --count=16";
my $md5 = "50f5f16e92e627def9f7d5f9a6e58b7b";

check_md5_output($md5, $cmd);
