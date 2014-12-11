#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwmatch

use strict;
use SiLKTests;

my $rwmatch = check_silk_app('rwmatch');
my $cmd = "$rwmatch";

exit (check_exit_status($cmd) ? 1 : 0);
