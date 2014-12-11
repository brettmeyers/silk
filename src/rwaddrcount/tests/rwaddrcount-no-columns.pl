#! /usr/bin/perl -w
# MD5: 8deb3b13b5ba85e9c7b27191da24c906
# TEST: ./rwaddrcount --print-rec --sort-ips --no-columns --no-title ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwaddrcount --print-rec --sort-ips --no-columns --no-title $file{data}";
my $md5 = "8deb3b13b5ba85e9c7b27191da24c906";

check_md5_output($md5, $cmd);
