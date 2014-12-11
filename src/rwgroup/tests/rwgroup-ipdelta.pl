#! /usr/bin/perl -w
# MD5: c9065a5284a62adbd5651fa9d52cebb7
# TEST: ../rwsort/rwsort --fields=1 ../../tests/data.rwf | ./rwgroup --delta-field=1 --delta-value=16 | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwsort --fields=1 $file{data} | $rwgroup --delta-field=1 --delta-value=16 | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "c9065a5284a62adbd5651fa9d52cebb7";

check_md5_output($md5, $cmd);
