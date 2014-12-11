#! /usr/bin/perl -w
# MD5: 4a1d670c84bcd15e29ce5a3fca077ec8
# TEST: ./rwsort --fields=dip ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwsort --fields=dip $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "4a1d670c84bcd15e29ce5a3fca077ec8";

check_md5_output($md5, $cmd);
