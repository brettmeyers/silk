#! /usr/bin/perl -w
# MD5: 15df26bd26755222c1f0c9d517b40d1e
# TEST: ./rwbag --sport-flows=stdout ../../tests/data.rwf | ./rwbagtool --mincounter=10 | ./rwbagcat --key-format=decimal

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sport-flows=stdout $file{data} | $rwbagtool --mincounter=10 | $rwbagcat --key-format=decimal";
my $md5 = "15df26bd26755222c1f0c9d517b40d1e";

check_md5_output($md5, $cmd);
