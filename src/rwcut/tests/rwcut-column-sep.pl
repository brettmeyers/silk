#! /usr/bin/perl -w
# MD5: e72638fd5ecf5e64eda1d49102e19bba
# TEST: ./rwcut --fields=5,4,3 --column-separator=, --no-columns ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=5,4,3 --column-separator=, --no-columns $file{data}";
my $md5 = "e72638fd5ecf5e64eda1d49102e19bba";

check_md5_output($md5, $cmd);
