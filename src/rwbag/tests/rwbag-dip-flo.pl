#! /usr/bin/perl -w
# MD5: 5971604c7c6cd3afdacd7e2ffc5f00e8
# TEST: ./rwbag --dip-flows=stdout ../../tests/data.rwf | ./rwbagcat --key-format=zero-padded

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --dip-flows=stdout $file{data} | $rwbagcat --key-format=zero-padded";
my $md5 = "5971604c7c6cd3afdacd7e2ffc5f00e8";

check_md5_output($md5, $cmd);
