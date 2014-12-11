#! /usr/bin/perl -w
# MD5: 14253d7dc187e114d05d4b8f46931b3b
# TEST: echo 10.10.10.10 | ./rwip2cc --map-file=../../tests/fake-cc.pmap --input-file=- --delimited=,

use strict;
use SiLKTests;

my $rwip2cc = check_silk_app('rwip2cc');
my %file;
$file{fake_cc} = get_data_or_exit77('fake_cc');
my $cmd = "echo 10.10.10.10 | $rwip2cc --map-file=$file{fake_cc} --input-file=- --delimited=,";
my $md5 = "14253d7dc187e114d05d4b8f46931b3b";

check_md5_output($md5, $cmd);
