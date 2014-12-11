#! /usr/bin/perl -w
# MD5: ca768d650b48fc42a4ee8534b643ecf7
# TEST: ./rwcount --bin-size=3600 --load-scheme=1 --timestamp-format=default ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=3600 --load-scheme=1 --timestamp-format=default $file{data}";
my $md5 = "ca768d650b48fc42a4ee8534b643ecf7";

check_md5_output($md5, $cmd);
