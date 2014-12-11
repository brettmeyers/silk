#! /usr/bin/perl -w
# MD5: c0fa23d581cf69438f01e841c74e2ab8
# TEST: cat ../../tests/data.rwf | ./rwnetmask --sip-prefix-length=24 | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwnetmask = check_silk_app('rwnetmask');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "cat $file{data} | $rwnetmask --sip-prefix-length=24 | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "c0fa23d581cf69438f01e841c74e2ab8";

check_md5_output($md5, $cmd);
