#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwpmapbuild --help

use strict;
use SiLKTests;

my $rwpmapbuild = check_silk_app('rwpmapbuild');
my $cmd = "$rwpmapbuild --help";

exit (check_exit_status($cmd) ? 0 : 1);
