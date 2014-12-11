#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwscanquery --version

use strict;
use SiLKTests;

my $rwscanquery = check_silk_app('rwscanquery');
my $cmd = "$rwscanquery --version";

exit (check_exit_status($cmd) ? 0 : 1);
