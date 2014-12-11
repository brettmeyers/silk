#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwfileinfo --version

use strict;
use SiLKTests;

my $rwfileinfo = check_silk_app('rwfileinfo');
my $cmd = "$rwfileinfo --version";

exit (check_exit_status($cmd) ? 0 : 1);
