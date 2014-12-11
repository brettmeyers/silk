#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwbagtool --version

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $cmd = "$rwbagtool --version";

exit (check_exit_status($cmd) ? 0 : 1);
