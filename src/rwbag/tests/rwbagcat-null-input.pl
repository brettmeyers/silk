#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwbagcat --minkey=50 --maxkey=100 </dev/null

use strict;
use SiLKTests;

my $rwbagcat = check_silk_app('rwbagcat');
my $cmd = "$rwbagcat --minkey=50 --maxkey=100 </dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
