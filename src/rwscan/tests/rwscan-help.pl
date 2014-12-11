#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwscan --help

use strict;
use SiLKTests;

my $rwscan = check_silk_app('rwscan');
my $cmd = "$rwscan --help";

exit (check_exit_status($cmd) ? 0 : 1);
