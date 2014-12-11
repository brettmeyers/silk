#! /usr/bin/perl -w
# MD5: 4230c82422d93f50cb973d6ea9eec1cd
# TEST: ./rwscan --scan-mode=2 ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwscan = check_silk_app('rwscan');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwscan --scan-mode=2 $file{empty}";
my $md5 = "4230c82422d93f50cb973d6ea9eec1cd";

check_md5_output($md5, $cmd);
