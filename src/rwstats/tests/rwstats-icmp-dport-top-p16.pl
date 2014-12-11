#! /usr/bin/perl -w
# MD5: b7eb674a49a5a452eee1aac031dd08cf
# TEST: ../rwfilter/rwfilter --dport=68 --fail=- ../../tests/data.rwf | ./rwstats --fields=proto,iType,iCode,dport --count=16

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --dport=68 --fail=- $file{data} | ./rwstats --fields=proto,iType,iCode,dport --count=16";
my $md5 = "b7eb674a49a5a452eee1aac031dd08cf";

check_md5_output($md5, $cmd);
