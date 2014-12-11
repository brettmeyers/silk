#! /usr/bin/perl -w
# MD5: ddd4b0caafd5cc25cbabb43d9d55cd11
# TEST: ./rwuniq --pmap-file=servhost:../../tests/ip-map.pmap --fields=dst-servhost --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwuniq --pmap-file=servhost:$file{ip_map} --fields=dst-servhost --sort-output $file{data}";
my $md5 = "ddd4b0caafd5cc25cbabb43d9d55cd11";

check_md5_output($md5, $cmd);
