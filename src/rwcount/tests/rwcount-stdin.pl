#! /usr/bin/perl -w
# MD5: ca768d650b48fc42a4ee8534b643ecf7
# TEST: cat ../../tests/data.rwf | ./rwcount --bin-size=3600 --load-scheme=1

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "cat $file{data} | $rwcount --bin-size=3600 --load-scheme=1";
my $md5 = "ca768d650b48fc42a4ee8534b643ecf7";

check_md5_output($md5, $cmd);
