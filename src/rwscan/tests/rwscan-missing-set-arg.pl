#! /usr/bin/perl -w
# ERR_MD5: 9b376dc347c8cd3fa2415607ba160127
# TEST: ./rwscan ../../tests/empty.rwf 2>&1

use strict;
use SiLKTests;

my $rwscan = check_silk_app('rwscan');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwscan $file{empty} 2>&1";
my $md5 = "9b376dc347c8cd3fa2415607ba160127";

check_md5_output($md5, $cmd, 1);
