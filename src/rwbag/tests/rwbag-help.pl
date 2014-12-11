#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwbag --help

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $cmd = "$rwbag --help";

exit (check_exit_status($cmd) ? 0 : 1);
