#! /usr/bin/perl -w
# MD5: 3051bb281de6a970c78085bda26ba2fc
# TEST: ./rwbag --sport-flows=stdout ../../tests/data.rwf | ./rwbagtool --maxcounter=10 | ./rwbagcat --key-format=decimal

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sport-flows=stdout $file{data} | $rwbagtool --maxcounter=10 | $rwbagcat --key-format=decimal";
my $md5 = "3051bb281de6a970c78085bda26ba2fc";

check_md5_output($md5, $cmd);
