#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwsort --help

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $cmd = "$rwsort --help";

exit (check_exit_status($cmd) ? 0 : 1);
