#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwbagbuild

use strict;
use SiLKTests;

my $rwbagbuild = check_silk_app('rwbagbuild');
my $cmd = "$rwbagbuild";

exit (check_exit_status($cmd) ? 1 : 0);
