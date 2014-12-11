#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwcompare --version

use strict;
use SiLKTests;

my $rwcompare = check_silk_app('rwcompare');
my $cmd = "$rwcompare --version";

exit (check_exit_status($cmd) ? 0 : 1);
