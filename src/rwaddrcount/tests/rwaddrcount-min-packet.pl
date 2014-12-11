#! /usr/bin/perl -w
# MD5: 47b0541443ebc5059ffec48e1e4cbcbc
# TEST: ./rwaddrcount --use-dest --print-rec --sort-ips --min-packet=20 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwaddrcount --use-dest --print-rec --sort-ips --min-packet=20 $file{data}";
my $md5 = "47b0541443ebc5059ffec48e1e4cbcbc";

check_md5_output($md5, $cmd);
