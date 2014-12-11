#! /usr/bin/perl -w
# MD5: 8d9f931e8e37efd312a6dd09ea9da0eb
# TEST: ./rwuniq --fields=proto --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=proto --sort-output $file{data}";
my $md5 = "8d9f931e8e37efd312a6dd09ea9da0eb";

check_md5_output($md5, $cmd);
