#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwsetbuild </dev/null >/dev/null

use strict;
use SiLKTests;

my $rwsetbuild = check_silk_app('rwsetbuild');
my $cmd = "$rwsetbuild </dev/null >/dev/null";

exit (check_exit_status($cmd) ? 0 : 1);
