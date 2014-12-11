#! /usr/bin/perl -w
# ERR_MD5: e3d528803ae2f0a86af3bc11b3d5c66c
# TEST: ./rwgroup --id-fields=3 ../../tests/data.rwf ../../tests/empty.rwf 2>&1

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my %file;
$file{data} = get_data_or_exit77('data');
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwgroup --id-fields=3 $file{data} $file{empty} 2>&1";
my $md5 = "e3d528803ae2f0a86af3bc11b3d5c66c";

check_md5_output($md5, $cmd, 1);
