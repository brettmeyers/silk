#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwbag

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $cmd = "$rwbag";

exit (check_exit_status($cmd) ? 1 : 0);
