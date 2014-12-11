#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwpmapbuild --version

use strict;
use SiLKTests;

my $rwpmapbuild = check_silk_app('rwpmapbuild');
my $cmd = "$rwpmapbuild --version";

exit (check_exit_status($cmd) ? 0 : 1);
