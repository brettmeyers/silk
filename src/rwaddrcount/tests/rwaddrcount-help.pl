#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwaddrcount --help

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my $cmd = "$rwaddrcount --help";

exit (check_exit_status($cmd) ? 0 : 1);
