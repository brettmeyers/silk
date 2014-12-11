#! /usr/bin/perl -w
# MD5: 1b647037fcf2a48eb976d5046a54d38d
# TEST: ./rwpmapcat --no-cidr-blocks --map-file ../../tests/ip-map.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwpmapcat --no-cidr-blocks --map-file $file{ip_map}";
my $md5 = "1b647037fcf2a48eb976d5046a54d38d";

check_md5_output($md5, $cmd);
