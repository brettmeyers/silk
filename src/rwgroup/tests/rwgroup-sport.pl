#! /usr/bin/perl -w
# MD5: e49ed3e324fcca7bed3f79588e9e0327
# TEST: ../rwsort/rwsort --fields=3 ../../tests/data.rwf | ./rwgroup --id-fields=3 | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwsort --fields=3 $file{data} | $rwgroup --id-fields=3 | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "e49ed3e324fcca7bed3f79588e9e0327";

check_md5_output($md5, $cmd);
