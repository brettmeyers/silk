#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwsiteinfo --help

use strict;
use SiLKTests;

my $rwsiteinfo = check_silk_app('rwsiteinfo');
my $cmd = "$rwsiteinfo --help";

exit (check_exit_status($cmd) ? 0 : 1);
