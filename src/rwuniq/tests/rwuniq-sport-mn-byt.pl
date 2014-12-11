#! /usr/bin/perl -w
# MD5: 0e39348130f91bf0d48fb626f340c1e8
# TEST: ./rwuniq --fields=sport --bytes=2000 --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=sport --bytes=2000 --sort-output $file{data}";
my $md5 = "0e39348130f91bf0d48fb626f340c1e8";

check_md5_output($md5, $cmd);
