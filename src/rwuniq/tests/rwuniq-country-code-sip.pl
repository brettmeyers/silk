#! /usr/bin/perl -w
# MD5: cf71295afdc2d2c677de915dc96514fd
# TEST: ./rwuniq --fields=scc --values=sip-distinct --ipv6=ignore --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
$file{fake_cc} = get_data_or_exit77('fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
my $cmd = "$rwuniq --fields=scc --values=sip-distinct --ipv6=ignore --sort-output $file{data}";
my $md5 = "cf71295afdc2d2c677de915dc96514fd";

check_md5_output($md5, $cmd);
