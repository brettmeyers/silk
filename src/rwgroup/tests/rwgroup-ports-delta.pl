#! /usr/bin/perl -w
# MD5: 9539f7d720a4d7c05187fa789b7e4696
# TEST: ../rwsort/rwsort --fields=3,4,9 ../../tests/data.rwf | ./rwgroup --id-fields=3,4 --delta-field=9 --delta-value=15 --objective | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwsort --fields=3,4,9 $file{data} | $rwgroup --id-fields=3,4 --delta-field=9 --delta-value=15 --objective | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "9539f7d720a4d7c05187fa789b7e4696";

check_md5_output($md5, $cmd);
