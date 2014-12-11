#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwtotal ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwtotal $file{empty}";

exit (check_exit_status($cmd) ? 1 : 0);
