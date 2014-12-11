#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwflowpack --help

use strict;
use SiLKTests;

my $rwflowpack = check_silk_app('rwflowpack');
my $cmd = "$rwflowpack --help";

exit (check_exit_status($cmd) ? 0 : 1);
