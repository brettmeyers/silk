#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwgroup --help

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $cmd = "$rwgroup --help";

exit (check_exit_status($cmd) ? 0 : 1);
