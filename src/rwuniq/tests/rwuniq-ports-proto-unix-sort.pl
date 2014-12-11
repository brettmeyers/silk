#! /usr/bin/perl -w
# MD5: cf89a898adcf8c160f270351dfd0469a
# TEST: ./rwuniq --fields=3-5 --no-title ../../tests/data.rwf | sort

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=3-5 --no-title $file{data} | sort";
my $md5 = "cf89a898adcf8c160f270351dfd0469a";

check_md5_output($md5, $cmd);
