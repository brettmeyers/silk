#! /usr/bin/perl -w
# MD5: 48a87ceae6ed7a867a98e9708060a3f0
# TEST: ./rwuniq --fields=sport --bytes=0-2000 --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=sport --bytes=0-2000 --sort-output $file{data}";
my $md5 = "48a87ceae6ed7a867a98e9708060a3f0";

check_md5_output($md5, $cmd);
