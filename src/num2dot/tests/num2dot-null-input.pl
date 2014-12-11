#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./num2dot </dev/null

use strict;
use SiLKTests;

my $num2dot = check_silk_app('num2dot');
my $cmd = "$num2dot </dev/null";

exit (check_exit_status($cmd) ? 0 : 1);
