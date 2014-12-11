#! /usr/bin/perl -w
# MD5: d026c790694cb866683d999ea0c6826d
# TEST: ./rwfilter --bytes=1-100 --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --bytes=1-100 --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "d026c790694cb866683d999ea0c6826d";

check_md5_output($md5, $cmd);
