#! /usr/bin/perl -w
# MD5: 9cf54071d5aa0781c33fffe12f18940d
# TEST: ./rwuniq --fields=sport --flows=0-10 --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=sport --flows=0-10 --sort-output $file{data}";
my $md5 = "9cf54071d5aa0781c33fffe12f18940d";

check_md5_output($md5, $cmd);
