#! /usr/bin/perl -w
# MD5: cf71295afdc2d2c677de915dc96514fd
# TEST: ./rwsort --fields=scc ../../tests/data-v6.rwf | ../rwuniq/rwuniq --fields=scc --values=distinct:sip --presorted-input

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_fake_cc} = get_data_or_exit77('v6_fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{v6_fake_cc}";
my $cmd = "$rwsort --fields=scc $file{v6data} | $rwuniq --fields=scc --values=distinct:sip --presorted-input";
my $md5 = "cf71295afdc2d2c677de915dc96514fd";

check_md5_output($md5, $cmd);
