#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwsender --help

use strict;
use SiLKTests;

my $rwsender = check_silk_app('rwsender');
my $cmd = "$rwsender --help";

exit (check_exit_status($cmd) ? 0 : 1);
