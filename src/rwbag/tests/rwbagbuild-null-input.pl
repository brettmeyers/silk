#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwbagbuild </dev/null

use strict;
use SiLKTests;

my $rwbagbuild = check_silk_app('rwbagbuild');
my $cmd = "$rwbagbuild </dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
