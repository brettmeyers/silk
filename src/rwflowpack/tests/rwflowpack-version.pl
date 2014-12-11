#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwflowpack --version

use strict;
use SiLKTests;

my $rwflowpack = check_silk_app('rwflowpack');
my $cmd = "$rwflowpack --version";

exit (check_exit_status($cmd) ? 0 : 1);
