#! /usr/bin/perl -w
# MD5: 8f5ec80677cd22f27476da11f02a8595
# TEST: ./rwcut --fields=2 --delimited --ip-format=zero-padded ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=2 --delimited --ip-format=zero-padded $file{data}";
my $md5 = "8f5ec80677cd22f27476da11f02a8595";

check_md5_output($md5, $cmd);
