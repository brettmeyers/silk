#! /usr/bin/perl -w
# MD5: fd74a4f0d22e1f73f44818cab1a04f66
# TEST: ../rwfilter/rwfilter --daddr=192.168.0.0/16 --pass=/tmp/rwscan-trw-only-in ../../tests/data.rwf && ../rwset/rwset --dip=/tmp/rwscan-trw-only-inset /tmp/rwscan-trw-only-in && ../rwsort/rwsort --fields=sip,proto,dip /tmp/rwscan-trw-only-in ../../tests/scandata.rwf | ./rwscan --scan-mode=1 --trw-sip-set=/tmp/rwscan-trw-only-inset

use strict;
use SiLKTests;

my $rwscan = check_silk_app('rwscan');
my $rwfilter = check_silk_app('rwfilter');
my $rwset = check_silk_app('rwset');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
$file{scandata} = get_data_or_exit77('scandata');
my %temp;
$temp{in} = make_tempname('in');
$temp{inset} = make_tempname('inset');
my $cmd = "$rwfilter --daddr=192.168.0.0/16 --pass=$temp{in} $file{data} && $rwset --dip=$temp{inset} $temp{in} && $rwsort --fields=sip,proto,dip $temp{in} $file{scandata} | $rwscan --scan-mode=1 --trw-sip-set=$temp{inset}";
my $md5 = "fd74a4f0d22e1f73f44818cab1a04f66";

check_md5_output($md5, $cmd);
