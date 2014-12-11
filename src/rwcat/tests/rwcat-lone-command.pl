#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwcat

use strict;
use SiLKTests;

my $rwcat = check_silk_app('rwcat');
my $cmd = "$rwcat";

exit (check_exit_status($cmd) ? 1 : 0);
