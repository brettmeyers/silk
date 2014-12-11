#! /usr/bin/perl -w
# MD5: 86a2003f3cef398d028447630ccd2d8b
# TEST: ./rwcut --delimited ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --delimited $file{data}";
my $md5 = "86a2003f3cef398d028447630ccd2d8b";

check_md5_output($md5, $cmd);
