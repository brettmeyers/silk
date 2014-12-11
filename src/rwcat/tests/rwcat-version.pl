#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwcat --version

use strict;
use SiLKTests;

my $rwcat = check_silk_app('rwcat');
my $cmd = "$rwcat --version";

exit (check_exit_status($cmd) ? 0 : 1);
