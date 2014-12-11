#! /usr/bin/perl -w
# MD5: 5bec172887009c1adadc4e6c939086f4
# TEST: ./rwuniq --fields=sport --sip-distinct --sort-output ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwuniq --fields=sport --sip-distinct --sort-output $file{v6data}";
my $md5 = "5bec172887009c1adadc4e6c939086f4";

check_md5_output($md5, $cmd);
