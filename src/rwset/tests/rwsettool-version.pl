#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwsettool --version

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $cmd = "$rwsettool --version";

exit (check_exit_status($cmd) ? 0 : 1);
