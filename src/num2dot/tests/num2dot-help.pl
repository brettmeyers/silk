#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./num2dot --help

use strict;
use SiLKTests;

my $num2dot = check_silk_app('num2dot');
my $cmd = "$num2dot --help";

exit (check_exit_status($cmd) ? 0 : 1);
