#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwfglob

use strict;
use SiLKTests;

my $rwfglob = check_silk_app('rwfglob');
my $cmd = "$rwfglob";

exit (check_exit_status($cmd) ? 1 : 0);
