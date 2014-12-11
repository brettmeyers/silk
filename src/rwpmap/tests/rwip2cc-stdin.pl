#! /usr/bin/perl -w
# MD5: b1fb98b1e4ffa872e0867f3889d69d87
# TEST: echo 10.10.10.10 | ./rwip2cc --map-file=../../tests/fake-cc.pmap --input-file=-

use strict;
use SiLKTests;

my $rwip2cc = check_silk_app('rwip2cc');
my %file;
$file{fake_cc} = get_data_or_exit77('fake_cc');
my $cmd = "echo 10.10.10.10 | $rwip2cc --map-file=$file{fake_cc} --input-file=-";
my $md5 = "b1fb98b1e4ffa872e0867f3889d69d87";

check_md5_output($md5, $cmd);
