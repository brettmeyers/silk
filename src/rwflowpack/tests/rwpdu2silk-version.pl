#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwpdu2silk --version

use strict;
use SiLKTests;

my $rwpdu2silk = check_silk_app('rwpdu2silk');
my $cmd = "$rwpdu2silk --version";

exit (check_exit_status($cmd) ? 0 : 1);
