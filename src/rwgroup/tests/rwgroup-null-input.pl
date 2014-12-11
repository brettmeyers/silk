#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwgroup --id-fields=3 /dev/null

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $cmd = "$rwgroup --id-fields=3 /dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
