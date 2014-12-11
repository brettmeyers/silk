#! /usr/bin/perl -w
# MD5: b0953356203710b12d315e8c5b4dffbe
# TEST: ./rwuniq --fields=sport --packets=0-20 --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=sport --packets=0-20 --sort-output $file{data}";
my $md5 = "b0953356203710b12d315e8c5b4dffbe";

check_md5_output($md5, $cmd);
