#! /usr/bin/perl -w
# MD5: bc63d751a4fc855cb6f70c067e4fae76
# TEST: ./rwuniq --fields=dcc --values=distinct:scc --sort-output ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_fake_cc} = get_data_or_exit77('v6_fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{v6_fake_cc}";
my $cmd = "$rwuniq --fields=dcc --values=distinct:scc --sort-output $file{v6data}";
my $md5 = "bc63d751a4fc855cb6f70c067e4fae76";

check_md5_output($md5, $cmd);
