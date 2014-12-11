#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwsender

use strict;
use SiLKTests;

my $rwsender = check_silk_app('rwsender');
my $cmd = "$rwsender";

exit (check_exit_status($cmd) ? 1 : 0);
