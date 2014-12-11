#! /usr/bin/perl -w
# MD5: 78ab9c729a76da08d03cb38f7fea2c46
# TEST: echo 10.10.10.10 | ./rwip2cc --map-file=../../tests/fake-cc.pmap --input-file=- --integer-ips --column-separator=/

use strict;
use SiLKTests;

my $rwip2cc = check_silk_app('rwip2cc');
my %file;
$file{fake_cc} = get_data_or_exit77('fake_cc');
my $cmd = "echo 10.10.10.10 | $rwip2cc --map-file=$file{fake_cc} --input-file=- --integer-ips --column-separator=/";
my $md5 = "78ab9c729a76da08d03cb38f7fea2c46";

check_md5_output($md5, $cmd);
