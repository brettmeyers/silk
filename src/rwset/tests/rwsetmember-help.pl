#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwsetmember --help

use strict;
use SiLKTests;

my $rwsetmember = check_silk_app('rwsetmember');
my $cmd = "$rwsetmember --help";

exit (check_exit_status($cmd) ? 0 : 1);
