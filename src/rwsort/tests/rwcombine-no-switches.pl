#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwcombine ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwcombine = check_silk_app('rwcombine');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwcombine $file{empty}";

exit (check_exit_status($cmd) ? 0 : 1);
