#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwallformats --help

use strict;
use SiLKTests;

my $rwallformats = check_silk_app('rwallformats');
my $cmd = "$rwallformats --help";

exit (check_exit_status($cmd) ? 0 : 1);
