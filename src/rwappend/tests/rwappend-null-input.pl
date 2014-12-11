#! /usr/bin/perl -w
# STATUS: OK
# TEST: cp ../../tests/empty.rwf /tmp/rwappend-null-input-out && ./rwappend --create /tmp/rwappend-null-input-out /dev/null

use strict;
use SiLKTests;

my $rwappend = check_silk_app('rwappend');
my %file;
$file{empty} = get_data_or_exit77('empty');
my %temp;
$temp{out} = make_tempname('out');
my $cmd = "cp $file{empty} $temp{out} && $rwappend --create $temp{out} /dev/null";

exit (check_exit_status($cmd) ? 0 : 1);
