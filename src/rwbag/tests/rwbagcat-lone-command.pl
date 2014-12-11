#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwbagcat

use strict;
use SiLKTests;

my $rwbagcat = check_silk_app('rwbagcat');
my $cmd = "$rwbagcat";

exit (check_exit_status($cmd) ? 1 : 0);
