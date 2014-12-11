#! /usr/bin/perl -w
# MD5: 15df26bd26755222c1f0c9d517b40d1e
# TEST: ./rwbag --sport-flows=stdout ../../tests/data.rwf | ./rwbagcat --key-format=decimal --mincounter=10

use strict;
use SiLKTests;

my $rwbagcat = check_silk_app('rwbagcat');
my $rwbag = check_silk_app('rwbag');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sport-flows=stdout $file{data} | $rwbagcat --key-format=decimal --mincounter=10";
my $md5 = "15df26bd26755222c1f0c9d517b40d1e";

check_md5_output($md5, $cmd);
