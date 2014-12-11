#! /usr/bin/perl -w
# MD5: 2e1d1afc790737485d6c655e902f78ba
# TEST: ./rwsort --fields=10 ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwsort --fields=10 $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "2e1d1afc790737485d6c655e902f78ba";

check_md5_output($md5, $cmd);
