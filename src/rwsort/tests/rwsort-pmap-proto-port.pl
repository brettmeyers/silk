#! /usr/bin/perl -w
# MD5: d5f3472e21a885bf5919ffd12c6fd267
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data.rwf | ./rwsort --pmap-file=../../tests/proto-port-map.pmap --fields=sval | ../rwuniq/rwuniq --pmap-file=../../tests/proto-port-map.pmap --fields=sval --presorted-input

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwfilter = check_silk_app('rwfilter');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{data} | $rwsort --pmap-file=$file{proto_port_map} --fields=sval | $rwuniq --pmap-file=$file{proto_port_map} --fields=sval --presorted-input";
my $md5 = "d5f3472e21a885bf5919ffd12c6fd267";

check_md5_output($md5, $cmd);
