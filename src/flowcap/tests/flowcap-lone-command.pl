#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./flowcap

use strict;
use SiLKTests;

my $flowcap = check_silk_app('flowcap');
my $cmd = "$flowcap";

exit (check_exit_status($cmd) ? 1 : 0);
