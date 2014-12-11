#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwrecgenerator --version

use strict;
use SiLKTests;

my $rwrecgenerator = check_silk_app('rwrecgenerator');
my $cmd = "$rwrecgenerator --version";

exit (check_exit_status($cmd) ? 0 : 1);
