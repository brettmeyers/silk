#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwfilter

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $cmd = "$rwfilter";

exit (check_exit_status($cmd) ? 1 : 0);
