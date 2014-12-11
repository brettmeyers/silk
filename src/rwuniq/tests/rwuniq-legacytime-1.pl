#! /usr/bin/perl -w
# MD5: fe4a972eeb8364177701806a13525537
# TEST: ./rwuniq --fields=9,11 --timestamp-format=m/d/y --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=9,11 --timestamp-format=m/d/y --sort-output $file{data}";
my $md5 = "fe4a972eeb8364177701806a13525537";

check_md5_output($md5, $cmd);
