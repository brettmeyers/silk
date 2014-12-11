#! /usr/bin/perl -w
# MD5: 8dae8a4e2ad9878a4c8f2817aa6cb925
# TEST: ./rwuniq --fields=sport --no-titles --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=sport --no-titles --sort-output $file{data}";
my $md5 = "8dae8a4e2ad9878a4c8f2817aa6cb925";

check_md5_output($md5, $cmd);
