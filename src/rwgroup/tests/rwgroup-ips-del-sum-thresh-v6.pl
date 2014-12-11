#! /usr/bin/perl -w
# MD5: 167c60529a2bbeaa40a678f242a80c0a
# TEST: ../rwsort/rwsort --fields=1,2,9 ../../tests/data-v6.rwf | ./rwgroup --id-fields=1,2 --delta-field=9 --delta-value=15 --summarize --rec-threshold=5 | ../rwcat/rwcat --compression-method=none --byte-order=little

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwsort --fields=1,2,9 $file{v6data} | $rwgroup --id-fields=1,2 --delta-field=9 --delta-value=15 --summarize --rec-threshold=5 | $rwcat --compression-method=none --byte-order=little";
my $md5 = "167c60529a2bbeaa40a678f242a80c0a";

check_md5_output($md5, $cmd);
