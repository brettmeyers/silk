#! /usr/bin/perl -w
# MD5: a4a90a93208b4cf6c8424932a813fd40
# TEST: ./rwuniq --fields=stime --packets --flows --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=stime --packets --flows --sort-output $file{data}";
my $md5 = "a4a90a93208b4cf6c8424932a813fd40";

check_md5_output($md5, $cmd);
