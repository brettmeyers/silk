#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwcombine --help

use strict;
use SiLKTests;

my $rwcombine = check_silk_app('rwcombine');
my $cmd = "$rwcombine --help";

exit (check_exit_status($cmd) ? 0 : 1);
