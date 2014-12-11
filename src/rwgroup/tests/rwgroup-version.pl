#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwgroup --version

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $cmd = "$rwgroup --version";

exit (check_exit_status($cmd) ? 0 : 1);
