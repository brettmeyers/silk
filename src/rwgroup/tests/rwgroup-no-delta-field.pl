#! /usr/bin/perl -w
# ERR_MD5: a87e10def94b028bf22bccff76a04d94
# TEST: ./rwgroup --id-field=3 --delta-value=10 ../../tests/empty.rwf 2>&1

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwgroup --id-field=3 --delta-value=10 $file{empty} 2>&1";
my $md5 = "a87e10def94b028bf22bccff76a04d94";

check_md5_output($md5, $cmd, 1);
