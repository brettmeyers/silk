#! /usr/bin/perl -w
# MD5: 6e72fd0103d60d0dfb3d876c7b4fdac5
# TEST: ./rwuniq --fields=iType,iCode,dport,proto --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=iType,iCode,dport,proto --sort-output $file{data}";
my $md5 = "6e72fd0103d60d0dfb3d876c7b4fdac5";

check_md5_output($md5, $cmd);
