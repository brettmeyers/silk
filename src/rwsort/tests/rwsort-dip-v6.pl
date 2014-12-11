#! /usr/bin/perl -w
# MD5: fb2abc3c8b7ec61c442412d2906672c4
# TEST: ./rwsort --fields=dip ../../tests/data-v6.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwsort --fields=dip $file{v6data} | $rwcat --compression-method=none --byte-order=little";
my $md5 = "fb2abc3c8b7ec61c442412d2906672c4";

check_md5_output($md5, $cmd);
