#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwpackchecker --help

use strict;
use SiLKTests;

my $rwpackchecker = check_silk_app('rwpackchecker');
my $cmd = "$rwpackchecker --help";

exit (check_exit_status($cmd) ? 0 : 1);
