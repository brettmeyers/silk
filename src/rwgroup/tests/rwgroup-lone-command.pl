#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwgroup

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $cmd = "$rwgroup";

exit (check_exit_status($cmd) ? 1 : 0);
