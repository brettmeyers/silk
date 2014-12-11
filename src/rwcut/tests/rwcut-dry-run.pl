#! /usr/bin/perl -w
# MD5: 8aa8c10e125c8e5507612586c4fa254e
# TEST: ./rwcut --dry-run --ipv6-policy=ignore ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --dry-run --ipv6-policy=ignore $file{data}";
my $md5 = "8aa8c10e125c8e5507612586c4fa254e";

check_md5_output($md5, $cmd);
