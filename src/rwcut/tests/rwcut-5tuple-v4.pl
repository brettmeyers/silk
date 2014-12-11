#! /usr/bin/perl -w
# MD5: 2d5ad3bead47fb2024e8c23e60f56682
# TEST: ./rwcut --fields=1-5 --ipv6-policy=ignore ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=1-5 --ipv6-policy=ignore $file{data}";
my $md5 = "2d5ad3bead47fb2024e8c23e60f56682";

check_md5_output($md5, $cmd);
