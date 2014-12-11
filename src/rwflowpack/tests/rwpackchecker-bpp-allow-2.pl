#! /usr/bin/perl -w
# MD5: d41d8cd98f00b204e9800998ecf8427e
# TEST: ./rwpackchecker --value max-tcp-bpp=5000 --allowable-count max-tcp-bpp=2 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwpackchecker = check_silk_app('rwpackchecker');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwpackchecker --value max-tcp-bpp=5000 --allowable-count max-tcp-bpp=2 $file{data}";
my $md5 = "d41d8cd98f00b204e9800998ecf8427e";

check_md5_output($md5, $cmd);
