#! /usr/bin/perl -w
# MD5: 6bd91d202c8785bd039df13ebae22efa
# TEST: ./rwstats --fields=etime --bin-time=3600 --values=bytes --count=100 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=etime --bin-time=3600 --values=bytes --count=100 $file{data}";
my $md5 = "6bd91d202c8785bd039df13ebae22efa";

check_md5_output($md5, $cmd);
