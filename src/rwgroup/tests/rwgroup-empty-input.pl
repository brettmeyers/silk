#! /usr/bin/perl -w
# MD5: d72d949264f8735075fdc3315b08bfb2
# TEST: ./rwgroup --id-fields=3 ../../tests/empty.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwgroup --id-fields=3 $file{empty} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "d72d949264f8735075fdc3315b08bfb2";

check_md5_output($md5, $cmd);
