#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwappend --create /dev/null ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwappend = check_silk_app('rwappend');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwappend --create /dev/null $file{empty}";

exit (check_exit_status($cmd) ? 1 : 0);
