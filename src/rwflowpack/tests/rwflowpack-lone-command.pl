#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwflowpack

use strict;
use SiLKTests;

my $rwflowpack = check_silk_app('rwflowpack');
my $cmd = "$rwflowpack";

exit (check_exit_status($cmd) ? 1 : 0);
