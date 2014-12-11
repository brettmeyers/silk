#! /usr/bin/perl -w
# ERR_MD5: d41d8cd98f00b204e9800998ecf8427e
# TEST: ./rwcompare --quiet ../../tests/empty.rwf ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcompare = check_silk_app('rwcompare');
my %file;
$file{data} = get_data_or_exit77('data');
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwcompare --quiet $file{empty} $file{data}";
my $md5 = "d41d8cd98f00b204e9800998ecf8427e";

check_md5_output($md5, $cmd, 1);
