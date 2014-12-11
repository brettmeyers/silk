#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwpackchecker --version

use strict;
use SiLKTests;

my $rwpackchecker = check_silk_app('rwpackchecker');
my $cmd = "$rwpackchecker --version";

exit (check_exit_status($cmd) ? 0 : 1);
