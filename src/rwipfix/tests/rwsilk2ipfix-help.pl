#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwsilk2ipfix --help

use strict;
use SiLKTests;

my $rwsilk2ipfix = check_silk_app('rwsilk2ipfix');
check_features(qw(ipfix));
my $cmd = "$rwsilk2ipfix --help";

exit (check_exit_status($cmd) ? 0 : 1);
