#! /usr/bin/perl -w
# MD5: 57c6c37c5bc522ff9a9227faf9ee9ebf
# TEST: ./rwuniq --fields=sport --sip-distinct --dip-distinct --sort-output ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwuniq --fields=sport --sip-distinct --dip-distinct --sort-output $file{v6data}";
my $md5 = "57c6c37c5bc522ff9a9227faf9ee9ebf";

check_md5_output($md5, $cmd);
