#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwbagtool

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $cmd = "$rwbagtool";

exit (check_exit_status($cmd) ? 1 : 0);
