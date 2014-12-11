#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwsetbuild --version

use strict;
use SiLKTests;

my $rwsetbuild = check_silk_app('rwsetbuild');
my $cmd = "$rwsetbuild --version";

exit (check_exit_status($cmd) ? 0 : 1);
