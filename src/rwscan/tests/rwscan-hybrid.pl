#! /usr/bin/perl -w
# MD5: c30d406c3e6b5be4af5ef38b8e7f5b2a
# TEST: ../rwfilter/rwfilter --daddr=192.168.0.0/16 --pass=/tmp/rwscan-hybrid-in ../../tests/data.rwf && ../rwset/rwset --dip=/tmp/rwscan-hybrid-inset /tmp/rwscan-hybrid-in && ../rwsort/rwsort --fields=sip,proto,dip /tmp/rwscan-hybrid-in ../../tests/scandata.rwf | ./rwscan --trw-sip-set=/tmp/rwscan-hybrid-inset

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
my $cmd = "$rwfilter --daddr=192.168.0.0/16 --pass=$temp{in} $file{data} && $rwset --dip=$temp{inset} $temp{in} && $rwsort --fields=sip,proto,dip $temp{in} $file{scandata} | $rwscan --trw-sip-set=$temp{inset}";
my $md5 = "c30d406c3e6b5be4af5ef38b8e7f5b2a";

check_md5_output($md5, $cmd);
