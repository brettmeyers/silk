#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwidsquery --help

use strict;
use SiLKTests;

my $rwidsquery = check_silk_app('rwidsquery');
my $cmd = "$rwidsquery --help";

exit (check_exit_status($cmd) ? 0 : 1);
