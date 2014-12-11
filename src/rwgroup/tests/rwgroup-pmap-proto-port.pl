#! /usr/bin/perl -w
# MD5: 32ccba8cb86dd0d635a955abcda1b932
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data.rwf | ../rwsort/rwsort --pmap-file=../../tests/proto-port-map.pmap --fields=sval | ./rwgroup --pmap-file=../../tests/proto-port-map.pmap --id-fields=sval | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwfilter = check_silk_app('rwfilter');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{data} | $rwsort --pmap-file=$file{proto_port_map} --fields=sval | $rwgroup --pmap-file=$file{proto_port_map} --id-fields=sval | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "32ccba8cb86dd0d635a955abcda1b932";

check_md5_output($md5, $cmd);
