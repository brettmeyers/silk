#! /usr/bin/perl -w
# MD5: eca41b3e3b768672bf4120558ddf0d49
# TEST: ./rwcut --fields=1 --delimited --ip-format=decimal ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=1 --delimited --ip-format=decimal $file{data}";
my $md5 = "eca41b3e3b768672bf4120558ddf0d49";

check_md5_output($md5, $cmd);
