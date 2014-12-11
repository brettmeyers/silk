#! /usr/bin/perl -w
# MD5: 1b647037fcf2a48eb976d5046a54d38d
# TEST: cat ../../tests/ip-map.pmap | ./rwpmapcat --map-file=- --no-cidr

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "cat $file{ip_map} | $rwpmapcat --map-file=- --no-cidr";
my $md5 = "1b647037fcf2a48eb976d5046a54d38d";

check_md5_output($md5, $cmd);
