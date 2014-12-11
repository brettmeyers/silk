#! /usr/bin/perl -w
# MD5: 57c6c37c5bc522ff9a9227faf9ee9ebf
# TEST: ../rwsort/rwsort --fields=sport ../../tests/data.rwf | ./rwuniq --fields=sport --sip-distinct --dip-distinct --presorted-input --ipv6-policy=ignore

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwsort --fields=sport $file{data} | $rwuniq --fields=sport --sip-distinct --dip-distinct --presorted-input --ipv6-policy=ignore";
my $md5 = "57c6c37c5bc522ff9a9227faf9ee9ebf";

check_md5_output($md5, $cmd);
