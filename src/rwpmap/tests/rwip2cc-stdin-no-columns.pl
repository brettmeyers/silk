#! /usr/bin/perl -w
# MD5: 2cf077a1bd736ae3d3b7b5db0c0f9c18
# TEST: echo 10.10.10.10 | ./rwip2cc --map-file=../../tests/fake-cc.pmap --input-file=- --no-columns

use strict;
use SiLKTests;

my $rwip2cc = check_silk_app('rwip2cc');
my %file;
$file{fake_cc} = get_data_or_exit77('fake_cc');
my $cmd = "echo 10.10.10.10 | $rwip2cc --map-file=$file{fake_cc} --input-file=- --no-columns";
my $md5 = "2cf077a1bd736ae3d3b7b5db0c0f9c18";

check_md5_output($md5, $cmd);
