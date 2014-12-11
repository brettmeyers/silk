#! /usr/bin/perl -w
# MD5: 8aa6d8f3d52d0745ea7134101b861ced
# TEST: ./rwbag --sport-flows=stdout ../../tests/data.rwf | ./rwbagtool --minkey=1024 | ./rwbagcat --key-format=decimal

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sport-flows=stdout $file{data} | $rwbagtool --minkey=1024 | $rwbagcat --key-format=decimal";
my $md5 = "8aa6d8f3d52d0745ea7134101b861ced";

check_md5_output($md5, $cmd);
