#! /usr/bin/perl -w
# MD5: 57c6c37c5bc522ff9a9227faf9ee9ebf
# TEST: ./rwuniq --fields=sport --sip-distinct --dip-distinct --ipv6-policy=ignore --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=sport --sip-distinct --dip-distinct --ipv6-policy=ignore --sort-output $file{data}";
my $md5 = "57c6c37c5bc522ff9a9227faf9ee9ebf";

check_md5_output($md5, $cmd);
