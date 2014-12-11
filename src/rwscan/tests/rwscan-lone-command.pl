#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwscan

use strict;
use SiLKTests;

my $rwscan = check_silk_app('rwscan');
my $cmd = "$rwscan";

exit (check_exit_status($cmd) ? 1 : 0);
