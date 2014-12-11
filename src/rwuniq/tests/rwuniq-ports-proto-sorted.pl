#! /usr/bin/perl -w
# MD5: cf89a898adcf8c160f270351dfd0469a
# TEST: ./rwuniq --fields=sport,dport,proto --no-title --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=sport,dport,proto --no-title --sort-output $file{data}";
my $md5 = "cf89a898adcf8c160f270351dfd0469a";

check_md5_output($md5, $cmd);
