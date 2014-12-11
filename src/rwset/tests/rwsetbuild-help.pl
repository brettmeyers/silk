#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwsetbuild --help

use strict;
use SiLKTests;

my $rwsetbuild = check_silk_app('rwsetbuild');
my $cmd = "$rwsetbuild --help";

exit (check_exit_status($cmd) ? 0 : 1);
