#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwrandomizeip ../../tests/empty.rwf </dev/null

use strict;
use SiLKTests;

my $rwrandomizeip = check_silk_app('rwrandomizeip');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwrandomizeip $file{empty} </dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
