#! /usr/bin/perl -w
# MD5: 835bf4e83f61c2201cc926a0b7656168
# TEST: ./rwfilter --saddress=2001:db8:a:fc-ff::x:x --fail=stdout ../../tests/data-v6.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwfilter --saddress=2001:db8:a:fc-ff::x:x --fail=stdout $file{v6data} | $rwcat --compression-method=none --byte-order=little";
my $md5 = "835bf4e83f61c2201cc926a0b7656168";

check_md5_output($md5, $cmd);
