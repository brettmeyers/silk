#! /usr/bin/perl -w
# MD5: d85679f067288864848e95a846e2f088
# TEST: echo 6/22 > /tmp/rwpmaplookup-files-proto-port-file1 ; echo 6/25 > /tmp/rwpmaplookup-files-proto-port-file2 ; echo 6/80 > /tmp/rwpmaplookup-files-proto-port-file3 ; ./rwpmaplookup --map-file=../../tests/proto-port-map.pmap /tmp/rwpmaplookup-files-proto-port-file1 /tmp/rwpmaplookup-files-proto-port-file2 /tmp/rwpmaplookup-files-proto-port-file3

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my %file;
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my %temp;
$temp{file1} = make_tempname('file1');
$temp{file2} = make_tempname('file2');
$temp{file3} = make_tempname('file3');
my $cmd = "echo 6/22 > $temp{file1} ; echo 6/25 > $temp{file2} ; echo 6/80 > $temp{file3} ; $rwpmaplookup --map-file=$file{proto_port_map} $temp{file1} $temp{file2} $temp{file3}";
my $md5 = "d85679f067288864848e95a846e2f088";

check_md5_output($md5, $cmd);
