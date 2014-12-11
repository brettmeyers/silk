#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwsort --version

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $cmd = "$rwsort --version";

exit (check_exit_status($cmd) ? 0 : 1);
