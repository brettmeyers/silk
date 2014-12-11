#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwsort

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $cmd = "$rwsort";

exit (check_exit_status($cmd) ? 1 : 0);
