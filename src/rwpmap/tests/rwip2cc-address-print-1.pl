#! /usr/bin/perl -w
# MD5: b1fb98b1e4ffa872e0867f3889d69d87
# TEST: ./rwip2cc --map-file=../../tests/fake-cc.pmap --print-ips=1 --address=10.10.10.10

use strict;
use SiLKTests;

my $rwip2cc = check_silk_app('rwip2cc');
my %file;
$file{fake_cc} = get_data_or_exit77('fake_cc');
my $cmd = "$rwip2cc --map-file=$file{fake_cc} --print-ips=1 --address=10.10.10.10";
my $md5 = "b1fb98b1e4ffa872e0867f3889d69d87";

check_md5_output($md5, $cmd);
