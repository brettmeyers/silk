#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwpmapbuild </dev/null

use strict;
use SiLKTests;

my $rwpmapbuild = check_silk_app('rwpmapbuild');
my $cmd = "$rwpmapbuild </dev/null";

exit (check_exit_status($cmd) ? 0 : 1);
