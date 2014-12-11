#! /usr/bin/perl -w
# MD5: 25a9d4633e6f90d5150e9de38d572417
# TEST: ./rwuniq --fields=dport --all-counts --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=dport --all-counts --sort-output $file{data}";
my $md5 = "25a9d4633e6f90d5150e9de38d572417";

check_md5_output($md5, $cmd);
