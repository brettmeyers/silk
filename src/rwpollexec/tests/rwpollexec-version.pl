#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwpollexec --version

use strict;
use SiLKTests;

my $rwpollexec = check_silk_app('rwpollexec');
my $cmd = "$rwpollexec --version";

exit (check_exit_status($cmd) ? 0 : 1);
