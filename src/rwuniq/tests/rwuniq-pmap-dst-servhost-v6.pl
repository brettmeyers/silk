#! /usr/bin/perl -w
# MD5: ddd4b0caafd5cc25cbabb43d9d55cd11
# TEST: ./rwuniq --pmap-file=servhost:../../tests/ip-map-v6.pmap --fields=dst-servhost --sort-output ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
check_features(qw(ipv6));
my $cmd = "$rwuniq --pmap-file=servhost:$file{v6_ip_map} --fields=dst-servhost --sort-output $file{v6data}";
my $md5 = "ddd4b0caafd5cc25cbabb43d9d55cd11";

check_md5_output($md5, $cmd);
