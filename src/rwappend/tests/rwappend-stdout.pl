#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwappend stdout ../../tests/empty.rwf >/dev/null

use strict;
use SiLKTests;

my $rwappend = check_silk_app('rwappend');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwappend stdout $file{empty} >/dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
