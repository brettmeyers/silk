#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwipaimport /dev/null

use strict;
use SiLKTests;

my $rwipaimport = check_silk_app('rwipaimport');
check_features(qw(ipa));
my $cmd = "$rwipaimport /dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
