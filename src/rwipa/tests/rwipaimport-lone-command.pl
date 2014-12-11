#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwipaimport

use strict;
use SiLKTests;

my $rwipaimport = check_silk_app('rwipaimport');
check_features(qw(ipa));
my $cmd = "$rwipaimport";

exit (check_exit_status($cmd) ? 1 : 0);
