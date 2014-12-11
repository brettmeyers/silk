#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwbagcat --version

use strict;
use SiLKTests;

my $rwbagcat = check_silk_app('rwbagcat');
my $cmd = "$rwbagcat --version";

exit (check_exit_status($cmd) ? 0 : 1);
