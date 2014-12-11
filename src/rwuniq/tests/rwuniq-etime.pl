#! /usr/bin/perl -w
# MD5: 99b324d3b11f13d558ef84e794e309d9
# TEST: ./rwuniq --fields=etime --timestamp-format=epoch --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=etime --timestamp-format=epoch --sort-output $file{data}";
my $md5 = "99b324d3b11f13d558ef84e794e309d9";

check_md5_output($md5, $cmd);
