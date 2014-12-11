#! /usr/bin/perl -w
# MD5: 68edbb3ee62df387bb487770291ba652
# TEST: ./rwsort --fields=5,3-4 ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwsort --fields=5,3-4 $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "68edbb3ee62df387bb487770291ba652";

check_md5_output($md5, $cmd);
