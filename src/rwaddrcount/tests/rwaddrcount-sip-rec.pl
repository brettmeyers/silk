#! /usr/bin/perl -w
# MD5: f794e2f1a999cf4a7c8ac539c299fc57
# TEST: ./rwaddrcount --print-rec ../../tests/data.rwf | sort

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwaddrcount --print-rec $file{data} | sort";
my $md5 = "f794e2f1a999cf4a7c8ac539c299fc57";

check_md5_output($md5, $cmd);
