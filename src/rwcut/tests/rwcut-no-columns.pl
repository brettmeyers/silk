#! /usr/bin/perl -w
# MD5: e2fd50fb3332823d10ff087cec796a44
# TEST: ./rwcut --fields=5,4,3 --no-columns ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=5,4,3 --no-columns $file{data}";
my $md5 = "e2fd50fb3332823d10ff087cec796a44";

check_md5_output($md5, $cmd);
