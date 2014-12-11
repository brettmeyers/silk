#! /usr/bin/perl -w
# MD5: 191a7cdb4172c1a6e747a6c01b263f2c
# TEST: ./rwuniq --fields=sport --no-column --column-sep=, --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=sport --no-column --column-sep=, --sort-output $file{data}";
my $md5 = "191a7cdb4172c1a6e747a6c01b263f2c";

check_md5_output($md5, $cmd);
