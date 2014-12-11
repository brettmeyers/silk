#! /usr/bin/perl -w
# MD5: 84c459a326c84a6a9423cc2196566019
# TEST: ./rwuniq --fields=sport --flows=10 --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=sport --flows=10 --sort-output $file{data}";
my $md5 = "84c459a326c84a6a9423cc2196566019";

check_md5_output($md5, $cmd);
