#! /usr/bin/perl -w
# MD5: e7d2c641630d0d24b280a2fca4606d55
# TEST: ../rwfilter/rwfilter --daddr=192.168.0.0/16 --pass=stdout ../../tests/data.rwf | ../rwsort/rwsort --fields=sip,proto,dip - ../../tests/scandata.rwf | ./rwscan --scan-mode=2

use strict;
use SiLKTests;

my $rwscan = check_silk_app('rwscan');
my $rwfilter = check_silk_app('rwfilter');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
$file{scandata} = get_data_or_exit77('scandata');
my %temp;
$temp{in} = make_tempname('in');
my $cmd = "$rwfilter --daddr=192.168.0.0/16 --pass=stdout $file{data} | $rwsort --fields=sip,proto,dip - $file{scandata} | $rwscan --scan-mode=2";
my $md5 = "e7d2c641630d0d24b280a2fca4606d55";

check_md5_output($md5, $cmd);
