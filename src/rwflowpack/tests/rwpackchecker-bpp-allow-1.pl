#! /usr/bin/perl -w
# ERR_MD5: 093071b7b9f46b84af7fea8c1ce54226
# TEST: ./rwpackchecker --value max-tcp-bpp=5000 --allowable-count max-tcp-bpp=1 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwpackchecker = check_silk_app('rwpackchecker');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwpackchecker --value max-tcp-bpp=5000 --allowable-count max-tcp-bpp=1 $file{data}";
my $md5 = "093071b7b9f46b84af7fea8c1ce54226";

check_md5_output($md5, $cmd, 1);
