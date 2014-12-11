#! /usr/bin/perl -w
# MD5: 9a7628f6bf4b3cfb1feaaf289bb62aca
# TEST: ./rwuniq --fields=sip --values=bytes --sort-output ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwuniq --fields=sip --values=bytes --sort-output $file{v6data}";
my $md5 = "9a7628f6bf4b3cfb1feaaf289bb62aca";

check_md5_output($md5, $cmd);
