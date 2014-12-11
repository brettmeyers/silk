#! /usr/bin/perl -w
# MD5: 074e543300d34ba3caec280a4e4c2175
# TEST: ./rwsort --fields=1 ../../tests/data-v6.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwsort --fields=1 $file{v6data} | $rwcat --compression-method=none --byte-order=little";
my $md5 = "074e543300d34ba3caec280a4e4c2175";

check_md5_output($md5, $cmd);
