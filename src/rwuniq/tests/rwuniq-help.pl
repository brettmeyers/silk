#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwuniq --help

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my $cmd = "$rwuniq --help";

exit (check_exit_status($cmd) ? 0 : 1);
