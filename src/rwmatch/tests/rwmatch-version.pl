#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwmatch --version

use strict;
use SiLKTests;

my $rwmatch = check_silk_app('rwmatch');
my $cmd = "$rwmatch --version";

exit (check_exit_status($cmd) ? 0 : 1);
