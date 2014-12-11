#! /usr/bin/perl -w
# MD5: baf2668fdcb58783f0db1b5d58eb955b
# TEST: echo 25,6 | ./rwfilter --tuple-file=- --tuple-delim=, --tuple-fields=sport,proto --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "echo 25,6 | $rwfilter --tuple-file=- --tuple-delim=, --tuple-fields=sport,proto --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "baf2668fdcb58783f0db1b5d58eb955b";

check_md5_output($md5, $cmd);
