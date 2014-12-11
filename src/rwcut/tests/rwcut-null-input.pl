#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwcut </dev/null

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my $cmd = "$rwcut </dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
