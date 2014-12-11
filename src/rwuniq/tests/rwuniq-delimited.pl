#! /usr/bin/perl -w
# MD5: 0252020803dc8eb45ad32cc55f01ca8e
# TEST: ./rwuniq --fields=sport --delimited --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=sport --delimited --sort-output $file{data}";
my $md5 = "0252020803dc8eb45ad32cc55f01ca8e";

check_md5_output($md5, $cmd);
