#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwfglob --help

use strict;
use SiLKTests;

my $rwfglob = check_silk_app('rwfglob');
my $cmd = "$rwfglob --help";

exit (check_exit_status($cmd) ? 0 : 1);
