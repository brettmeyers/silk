#! /usr/bin/perl -w
# MD5: c5e1d99bff97b9de64b26d57184321ee
# TEST: ./rwuniq --fields=dur --bytes --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=dur --bytes --sort-output $file{data}";
my $md5 = "c5e1d99bff97b9de64b26d57184321ee";

check_md5_output($md5, $cmd);
