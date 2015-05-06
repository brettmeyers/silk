#! /usr/bin/perl -w
# STATUS: ERR
# TEST: touch /tmp/rwappend-null-output-in && ./rwappend --create /tmp/rwappend-null-output-in ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwappend = check_silk_app('rwappend');
my %file;
$file{empty} = get_data_or_exit77('empty');
my %temp;
$temp{in} = make_tempname('in');
my $cmd = "touch $temp{in} && $rwappend --create $temp{in} $file{empty}";

exit (check_exit_status($cmd) ? 1 : 0);
