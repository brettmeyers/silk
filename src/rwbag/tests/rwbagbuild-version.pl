#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwbagbuild --version

use strict;
use SiLKTests;

my $rwbagbuild = check_silk_app('rwbagbuild');
my $cmd = "$rwbagbuild --version";

exit (check_exit_status($cmd) ? 0 : 1);
