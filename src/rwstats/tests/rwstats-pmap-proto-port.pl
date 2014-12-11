#! /usr/bin/perl -w
# MD5: f3bebe2db8f546b1f37de3b0343e4816
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data.rwf | ./rwstats --pmap-file=../../tests/proto-port-map.pmap --fields=sval --bottom --count=10

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{data} = get_data_or_exit77('data');
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{data} | $rwstats --pmap-file=$file{proto_port_map} --fields=sval --bottom --count=10";
my $md5 = "f3bebe2db8f546b1f37de3b0343e4816";

check_md5_output($md5, $cmd);
