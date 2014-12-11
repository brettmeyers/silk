#! /usr/bin/perl -w
# MD5: 1e7daa07b835d569c3e13e723b1d6c26
# TEST: cat ../../tests/data.rwf | ./rwaddrcount --print-rec --use-dest --sort-ips

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "cat $file{data} | $rwaddrcount --print-rec --use-dest --sort-ips";
my $md5 = "1e7daa07b835d569c3e13e723b1d6c26";

check_md5_output($md5, $cmd);
