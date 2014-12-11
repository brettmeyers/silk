#! /usr/bin/perl -w
# MD5: b368d2128dd5a33b2c48c5f46e5b1f38
# TEST: ./rwuniq --fields=scc --values=distinct:dcc --sort-output ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_fake_cc} = get_data_or_exit77('v6_fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{v6_fake_cc}";
my $cmd = "$rwuniq --fields=scc --values=distinct:dcc --sort-output $file{v6data}";
my $md5 = "b368d2128dd5a33b2c48c5f46e5b1f38";

check_md5_output($md5, $cmd);
