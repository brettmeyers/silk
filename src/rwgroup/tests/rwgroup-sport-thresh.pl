#! /usr/bin/perl -w
# MD5: 7d4fc97050bf1120c0736cfd813c0be3
# TEST: ../rwsort/rwsort --fields=3 ../../tests/data.rwf | ./rwgroup --id-fields=3 --rec-threshold=20 --group-offset=0.1.0.0 | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwsort --fields=3 $file{data} | $rwgroup --id-fields=3 --rec-threshold=20 --group-offset=0.1.0.0 | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "7d4fc97050bf1120c0736cfd813c0be3";

check_md5_output($md5, $cmd);
