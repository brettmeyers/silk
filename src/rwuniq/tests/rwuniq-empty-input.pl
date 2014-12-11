#! /usr/bin/perl -w
# MD5: 318c4ea2128a1d2bdc45a98208dd773c
# TEST: ./rwuniq --fields=sport --sort-output ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwuniq --fields=sport --sort-output $file{empty}";
my $md5 = "318c4ea2128a1d2bdc45a98208dd773c";

check_md5_output($md5, $cmd);
