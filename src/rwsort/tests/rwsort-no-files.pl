#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwsort --fields=1

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $cmd = "$rwsort --fields=1";

exit (check_exit_status($cmd) ? 1 : 0);
