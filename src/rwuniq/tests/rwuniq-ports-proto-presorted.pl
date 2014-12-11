#! /usr/bin/perl -w
# MD5: cf89a898adcf8c160f270351dfd0469a
# TEST: ../rwsort/rwsort --fields=3-5 ../../tests/data.rwf | ./rwuniq --fields=3-5 --presorted-input --no-title

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwsort --fields=3-5 $file{data} | $rwuniq --fields=3-5 --presorted-input --no-title";
my $md5 = "cf89a898adcf8c160f270351dfd0469a";

check_md5_output($md5, $cmd);
