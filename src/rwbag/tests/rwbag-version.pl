#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwbag --version

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $cmd = "$rwbag --version";

exit (check_exit_status($cmd) ? 0 : 1);
