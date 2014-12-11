#! /usr/bin/perl -w
# MD5: 87278962775ce9f216102a6f41f421f9
# TEST: echo 10.10.10.10 | ./rwip2cc --map-file=../../tests/fake-cc.pmap --input-file=- --zero-pad-ips --no-final-delimiter

use strict;
use SiLKTests;

my $rwip2cc = check_silk_app('rwip2cc');
my %file;
$file{fake_cc} = get_data_or_exit77('fake_cc');
my $cmd = "echo 10.10.10.10 | $rwip2cc --map-file=$file{fake_cc} --input-file=- --zero-pad-ips --no-final-delimiter";
my $md5 = "87278962775ce9f216102a6f41f421f9";

check_md5_output($md5, $cmd);
