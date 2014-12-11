#! /usr/bin/perl -w
# MD5: bd5cb35a584f39cbade68a5e7496e842
# TEST: ./rwsetcat --ip-ranges ../../tests/set1-v4.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
my $cmd = "$rwsetcat --ip-ranges $file{v4set1}";
my $md5 = "bd5cb35a584f39cbade68a5e7496e842";

check_md5_output($md5, $cmd);
