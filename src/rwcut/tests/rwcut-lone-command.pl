#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwcut

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my $cmd = "$rwcut";

exit (check_exit_status($cmd) ? 1 : 0);
