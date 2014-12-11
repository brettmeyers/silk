#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwfileinfo

use strict;
use SiLKTests;

my $rwfileinfo = check_silk_app('rwfileinfo');
my $cmd = "$rwfileinfo";

exit (check_exit_status($cmd) ? 1 : 0);
