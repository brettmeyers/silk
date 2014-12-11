#! /usr/bin/perl -w
# MD5: d72d949264f8735075fdc3315b08bfb2
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data-v6.rwf | ../rwsort/rwsort --pmap-file=../../tests/ip-map-v6.pmap --fields=src-service-host | ./rwgroup --pmap-file=../../tests/ip-map-v6.pmap --id-fields=src-service-host | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwfilter = check_silk_app('rwfilter');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
check_features(qw(ipv6));
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{v6data} | $rwsort --pmap-file=$file{v6_ip_map} --fields=src-service-host | $rwgroup --pmap-file=$file{v6_ip_map} --id-fields=src-service-host | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "d72d949264f8735075fdc3315b08bfb2";

check_md5_output($md5, $cmd);
