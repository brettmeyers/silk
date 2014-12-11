#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwpackchecker

use strict;
use SiLKTests;

my $rwpackchecker = check_silk_app('rwpackchecker');
my $cmd = "$rwpackchecker";

exit (check_exit_status($cmd) ? 1 : 0);
