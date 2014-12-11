#! /usr/bin/perl -w
# MD5: e184b517965ff483068b7d206d04b06d
# TEST: ./rwcut --all-fields --delimited ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --all-fields --delimited $file{data}";
my $md5 = "e184b517965ff483068b7d206d04b06d";

check_md5_output($md5, $cmd);
