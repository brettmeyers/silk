#! /usr/bin/perl -w
# MD5: 393789257810fde6263977f90d106343
# TEST: cp ../../tests/data.rwf /tmp/rwappend-create-exists-out && ./rwappend --create /tmp/rwappend-create-exists-out ../../tests/empty.rwf ../../tests/empty.rwf && ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output /tmp/rwappend-create-exists-out

use strict;
use SiLKTests;

my $rwappend = check_silk_app('rwappend');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{empty} = get_data_or_exit77('empty');
$file{data} = get_data_or_exit77('data');
my %temp;
$temp{out} = make_tempname('out');
my $cmd = "cp $file{data} $temp{out} && $rwappend --create $temp{out} $file{empty} $file{empty} && $rwcat --compression-method=none --byte-order=little --ipv4-output $temp{out}";
my $md5 = "393789257810fde6263977f90d106343";

check_md5_output($md5, $cmd);
