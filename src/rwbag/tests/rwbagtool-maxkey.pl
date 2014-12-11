#! /usr/bin/perl -w
# MD5: a7df9d39382715b6ed361764c439af25
# TEST: ./rwbag --sport-flows=stdout ../../tests/data.rwf | ./rwbagtool --maxkey=1024 | ./rwbagcat --key-format=decimal

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sport-flows=stdout $file{data} | $rwbagtool --maxkey=1024 | $rwbagcat --key-format=decimal";
my $md5 = "a7df9d39382715b6ed361764c439af25";

check_md5_output($md5, $cmd);
