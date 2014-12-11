#! /usr/bin/perl -w
# MD5: cc6191757c3dc8d2d2d0a4aaa2453e0e
# TEST: ./rwuniq --fields=dport,iType,iCode,proto --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=dport,iType,iCode,proto --sort-output $file{data}";
my $md5 = "cc6191757c3dc8d2d2d0a4aaa2453e0e";

check_md5_output($md5, $cmd);
