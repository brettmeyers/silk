#! /usr/bin/perl -w
# MD5: 1eb6f8a44c14e5c370a05d80244d9cac
# TEST: ./rwsetcat --count-ips --print-filenames ../../tests/set1-v4.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
my $cmd = "$rwsetcat --count-ips --print-filenames $file{v4set1}";
my $md5 = "1eb6f8a44c14e5c370a05d80244d9cac";

check_md5_output($md5, $cmd);
