#! /usr/bin/perl -w
# MD5: 4a5a76df0bf8cdf8144569e3f41b52a6
# TEST: ./rwuniq --fields=dip --ipv6-policy=ignore --ip-format=zero-padded --packets --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=dip --ipv6-policy=ignore --ip-format=zero-padded --packets --sort-output $file{data}";
my $md5 = "4a5a76df0bf8cdf8144569e3f41b52a6";

check_md5_output($md5, $cmd);
