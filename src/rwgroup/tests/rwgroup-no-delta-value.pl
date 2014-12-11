#! /usr/bin/perl -w
# ERR_MD5: 67254f32079d827290dd78a72340c07e
# TEST: ./rwgroup --delta-field=9 ../../tests/empty.rwf 2>&1

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwgroup --delta-field=9 $file{empty} 2>&1";
my $md5 = "67254f32079d827290dd78a72340c07e";

check_md5_output($md5, $cmd, 1);
