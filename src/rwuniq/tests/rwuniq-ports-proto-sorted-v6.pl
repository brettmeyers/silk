#! /usr/bin/perl -w
# MD5: 880eb330d6ee83f0870acba792f0530c
# TEST: ./rwuniq --fields=sport,dport,proto --no-title --sort-output ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwuniq --fields=sport,dport,proto --no-title --sort-output $file{v6data}";
my $md5 = "880eb330d6ee83f0870acba792f0530c";

check_md5_output($md5, $cmd);
