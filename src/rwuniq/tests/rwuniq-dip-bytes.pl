#! /usr/bin/perl -w
# MD5: 31d5ab1a3d45d90798add0a3ebe69b9c
# TEST: ./rwuniq --fields=2 --ipv6-policy=ignore --ip-format=decimal --bytes --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=2 --ipv6-policy=ignore --ip-format=decimal --bytes --sort-output $file{data}";
my $md5 = "31d5ab1a3d45d90798add0a3ebe69b9c";

check_md5_output($md5, $cmd);
