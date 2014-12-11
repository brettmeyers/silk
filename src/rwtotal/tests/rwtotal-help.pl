#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwtotal --help

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my $cmd = "$rwtotal --help";

exit (check_exit_status($cmd) ? 0 : 1);
