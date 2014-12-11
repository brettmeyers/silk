#! /usr/bin/perl -w
# MD5: c07f8a640b2cc1e0ea36e51a6aee5e93
# TEST: ../rwsort/rwsort --fields=1,2,9 ../../tests/data.rwf | ./rwgroup --id-fields=1,2 --delta-field=9 --delta-value=15 --summarize | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwsort --fields=1,2,9 $file{data} | $rwgroup --id-fields=1,2 --delta-field=9 --delta-value=15 --summarize | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "c07f8a640b2cc1e0ea36e51a6aee5e93";

check_md5_output($md5, $cmd);
