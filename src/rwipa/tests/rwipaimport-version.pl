#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwipaimport --version

use strict;
use SiLKTests;

my $rwipaimport = check_silk_app('rwipaimport');
check_features(qw(ipa));
my $cmd = "$rwipaimport --version";

exit (check_exit_status($cmd) ? 0 : 1);
