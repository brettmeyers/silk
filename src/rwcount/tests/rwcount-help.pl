#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwcount --help

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my $cmd = "$rwcount --help";

exit (check_exit_status($cmd) ? 0 : 1);
