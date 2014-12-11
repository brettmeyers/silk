#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwsettool

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $cmd = "$rwsettool";

exit (check_exit_status($cmd) ? 1 : 0);
