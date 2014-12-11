#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwsiteinfo

use strict;
use SiLKTests;

my $rwsiteinfo = check_silk_app('rwsiteinfo');
my $cmd = "$rwsiteinfo";

exit (check_exit_status($cmd) ? 1 : 0);
