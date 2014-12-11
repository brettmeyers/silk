#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwipaexport --version

use strict;
use SiLKTests;

my $rwipaexport = check_silk_app('rwipaexport');
check_features(qw(ipa));
my $cmd = "$rwipaexport --version";

exit (check_exit_status($cmd) ? 0 : 1);
