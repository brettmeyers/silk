#! /usr/bin/perl -w
# MD5: bde4b253f86ed148ec44a7a9f53842c2
# TEST: ./rwnetmask --6dip-prefix=64 --6sip-prefix=120 ../../tests/data-v6.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little

use strict;
use SiLKTests;

my $rwnetmask = check_silk_app('rwnetmask');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwnetmask --6dip-prefix=64 --6sip-prefix=120 $file{v6data} | $rwcat --compression-method=none --byte-order=little";
my $md5 = "bde4b253f86ed148ec44a7a9f53842c2";

check_md5_output($md5, $cmd);
