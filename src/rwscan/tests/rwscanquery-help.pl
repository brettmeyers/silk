#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwscanquery --help

use strict;
use SiLKTests;

my $rwscanquery = check_silk_app('rwscanquery');
my $cmd = "$rwscanquery --help";

exit (check_exit_status($cmd) ? 0 : 1);
