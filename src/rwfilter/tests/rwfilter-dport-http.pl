#! /usr/bin/perl -w
# MD5: 9949de1a9c39f0eb8b1e9ee6b9ebfa69
# TEST: ./rwfilter --pmap-file=service:../../tests/proto-port-map.pmap --pmap-dst-service=TCP/HTTP,TCP/HTTPS --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwfilter --pmap-file=service:$file{proto_port_map} --pmap-dst-service=TCP/HTTP,TCP/HTTPS --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "9949de1a9c39f0eb8b1e9ee6b9ebfa69";

check_md5_output($md5, $cmd);
