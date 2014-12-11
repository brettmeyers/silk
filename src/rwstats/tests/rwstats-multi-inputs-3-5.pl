#! /usr/bin/perl -w
# MD5: abbfee3a69381cf8d4ed024ba6f98d30
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=/tmp/rwstats-multi-inputs-3-5-in --fail=/tmp/rwstats-multi-inputs-3-5-out ../../tests/data.rwf && ./rwstats --fields=3-5 --values=bytes,packets  --threshold=30000000 --no-percents /tmp/rwstats-multi-inputs-3-5-in /tmp/rwstats-multi-inputs-3-5-out

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{data} = get_data_or_exit77('data');
my %temp;
$temp{in} = make_tempname('in');
$temp{out} = make_tempname('out');
my $cmd = "$rwfilter --type=in,inweb --pass=$temp{in} --fail=$temp{out} $file{data} && $rwstats --fields=3-5 --values=bytes,packets  --threshold=30000000 --no-percents $temp{in} $temp{out}";
my $md5 = "abbfee3a69381cf8d4ed024ba6f98d30";

check_md5_output($md5, $cmd);
