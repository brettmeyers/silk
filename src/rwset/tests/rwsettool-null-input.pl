#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwsettool </dev/null >/dev/null

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $cmd = "$rwsettool </dev/null >/dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
