#! /usr/bin/perl -w
# MD5: 393789257810fde6263977f90d106343
# TEST: ../rwcat/rwcat --byte-order=big ../../tests/empty.rwf > /tmp/rwappend-one-file-big-out && ./rwappend /tmp/rwappend-one-file-big-out ../../tests/data.rwf && ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output /tmp/rwappend-one-file-big-out

use strict;
use SiLKTests;

my $rwappend = check_silk_app('rwappend');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{empty} = get_data_or_exit77('empty');
$file{data} = get_data_or_exit77('data');
my %temp;
$temp{out} = make_tempname('out');
my $cmd = "$rwcat --byte-order=big $file{empty} > $temp{out} && $rwappend $temp{out} $file{data} && $rwcat --compression-method=none --byte-order=little --ipv4-output $temp{out}";
my $md5 = "393789257810fde6263977f90d106343";

check_md5_output($md5, $cmd);
