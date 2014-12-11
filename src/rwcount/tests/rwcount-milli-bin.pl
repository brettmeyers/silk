#! /usr/bin/perl -w
# MD5: 2c7da978f6efb777cf8bf9de64c2120a
# TEST: ./rwcount --bin-size=0.1 --load-scheme=2 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=0.1 --load-scheme=2 $file{data}";
my $md5 = "2c7da978f6efb777cf8bf9de64c2120a";

check_md5_output($md5, $cmd);
