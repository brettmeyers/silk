#! /usr/bin/perl -w
# MD5: 781378cfb8232cb3cd61977760d6ac86
# TEST: ./rwbag --sip-flows=stdout ../../tests/data.rwf | ./rwbagcat --network-structure=ATS

use strict;
use SiLKTests;

my $rwbagcat = check_silk_app('rwbagcat');
my $rwbag = check_silk_app('rwbag');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sip-flows=stdout $file{data} | $rwbagcat --network-structure=ATS";
my $md5 = "781378cfb8232cb3cd61977760d6ac86";

check_md5_output($md5, $cmd);
