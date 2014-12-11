#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./flowcap --help

use strict;
use SiLKTests;

my $flowcap = check_silk_app('flowcap');
my $cmd = "$flowcap --help";

exit (check_exit_status($cmd) ? 0 : 1);
