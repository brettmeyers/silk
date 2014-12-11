#! /usr/bin/perl -w
# MD5: 931b0bc37b1a25ead2855f8f883d0c7e
# TEST: ./rwaddrcount --print-rec --use-dest --sort-ips ../../tests/data.rwf ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwaddrcount --print-rec --use-dest --sort-ips $file{data} $file{data}";
my $md5 = "931b0bc37b1a25ead2855f8f883d0c7e";

check_md5_output($md5, $cmd);
