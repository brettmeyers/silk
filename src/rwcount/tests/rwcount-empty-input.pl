#! /usr/bin/perl -w
# MD5: c567b46a1e56d38962d57a3c8e88b00a
# TEST: ./rwcount ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwcount $file{empty}";
my $md5 = "c567b46a1e56d38962d57a3c8e88b00a";

check_md5_output($md5, $cmd);
