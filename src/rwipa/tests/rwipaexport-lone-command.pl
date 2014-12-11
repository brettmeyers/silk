#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwipaexport

use strict;
use SiLKTests;

my $rwipaexport = check_silk_app('rwipaexport');
check_features(qw(ipa));
my $cmd = "$rwipaexport";

exit (check_exit_status($cmd) ? 1 : 0);
