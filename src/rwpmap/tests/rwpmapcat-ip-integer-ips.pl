#! /usr/bin/perl -w
# MD5: f8d7865ebedea359cd9c27baa748a3e8
# TEST: ./rwpmapcat --ip-format=decimal --no-columns --map-file ../../tests/ip-map.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwpmapcat --ip-format=decimal --no-columns --map-file $file{ip_map}";
my $md5 = "f8d7865ebedea359cd9c27baa748a3e8";

check_md5_output($md5, $cmd);
