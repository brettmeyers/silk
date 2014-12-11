#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwpdu2silk --help

use strict;
use SiLKTests;

my $rwpdu2silk = check_silk_app('rwpdu2silk');
my $cmd = "$rwpdu2silk --help";

exit (check_exit_status($cmd) ? 0 : 1);
