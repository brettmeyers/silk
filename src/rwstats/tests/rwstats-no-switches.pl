#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwstats ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwstats $file{empty}";

exit (check_exit_status($cmd) ? 1 : 0);
