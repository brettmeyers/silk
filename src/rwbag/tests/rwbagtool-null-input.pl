#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwbagtool --minkey=50 --maxkey=100 </dev/null

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $cmd = "$rwbagtool --minkey=50 --maxkey=100 </dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
