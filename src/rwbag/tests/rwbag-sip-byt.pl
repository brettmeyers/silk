#! /usr/bin/perl -w
# MD5: f841785d50322a7b7fa01521a7f69dd6
# TEST: ./rwbag --sip-bytes=stdout ../../tests/data.rwf | ./rwbagcat

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sip-bytes=stdout $file{data} | $rwbagcat";
my $md5 = "f841785d50322a7b7fa01521a7f69dd6";

check_md5_output($md5, $cmd);
