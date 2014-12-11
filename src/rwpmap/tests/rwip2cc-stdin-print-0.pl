#! /usr/bin/perl -w
# MD5: df35e35989e712fa96925b955d6409ac
# TEST: echo 10.10.10.10 | ./rwip2cc --map-file=../../tests/fake-cc.pmap --input-file=- --print-ips=0

use strict;
use SiLKTests;

my $rwip2cc = check_silk_app('rwip2cc');
my %file;
$file{fake_cc} = get_data_or_exit77('fake_cc');
my $cmd = "echo 10.10.10.10 | $rwip2cc --map-file=$file{fake_cc} --input-file=- --print-ips=0";
my $md5 = "df35e35989e712fa96925b955d6409ac";

check_md5_output($md5, $cmd);
