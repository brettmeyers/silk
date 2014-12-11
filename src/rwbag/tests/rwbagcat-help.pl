#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwbagcat --help

use strict;
use SiLKTests;

my $rwbagcat = check_silk_app('rwbagcat');
my $cmd = "$rwbagcat --help";

exit (check_exit_status($cmd) ? 0 : 1);
