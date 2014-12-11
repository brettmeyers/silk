#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwipaexport --catalog=my-cat --time=2009/02/14:00:00 /dev/null

use strict;
use SiLKTests;

my $rwipaexport = check_silk_app('rwipaexport');
check_features(qw(ipa));
my $cmd = "$rwipaexport --catalog=my-cat --time=2009/02/14:00:00 /dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
