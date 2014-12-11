#! /usr/bin/perl -w
# MD5: 87d85ad6ed92bafc6ac547cad171e443
# TEST: ./rwpmapcat --left-justify-labels ../../tests/ip-map.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwpmapcat --left-justify-labels $file{ip_map}";
my $md5 = "87d85ad6ed92bafc6ac547cad171e443";

check_md5_output($md5, $cmd);
