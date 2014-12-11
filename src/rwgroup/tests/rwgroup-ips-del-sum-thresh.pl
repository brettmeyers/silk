#! /usr/bin/perl -w
# MD5: d72d949264f8735075fdc3315b08bfb2
# TEST: ../rwsort/rwsort --fields=1,2,9 ../../tests/data.rwf | ./rwgroup --id-fields=1,2 --delta-field=9 --delta-value=15 --summarize --rec-threshold=5 | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwsort --fields=1,2,9 $file{data} | $rwgroup --id-fields=1,2 --delta-field=9 --delta-value=15 --summarize --rec-threshold=5 | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "d72d949264f8735075fdc3315b08bfb2";

check_md5_output($md5, $cmd);
