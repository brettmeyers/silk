#! /usr/bin/perl -w
# MD5: 0c4226580aaa908976bcdd3aac8f9de9
# TEST: ../rwsort/rwsort --fields=1,2,9 ../../tests/data.rwf | ./rwgroup --id-fields=1,2 | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwsort --fields=1,2,9 $file{data} | $rwgroup --id-fields=1,2 | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "0c4226580aaa908976bcdd3aac8f9de9";

check_md5_output($md5, $cmd);
