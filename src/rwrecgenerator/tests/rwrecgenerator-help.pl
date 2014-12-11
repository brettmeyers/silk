#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwrecgenerator --help

use strict;
use SiLKTests;

my $rwrecgenerator = check_silk_app('rwrecgenerator');
my $cmd = "$rwrecgenerator --help";

exit (check_exit_status($cmd) ? 0 : 1);
