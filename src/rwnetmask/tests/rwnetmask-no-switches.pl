#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwnetmask ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwnetmask = check_silk_app('rwnetmask');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwnetmask $file{empty}";

exit (check_exit_status($cmd) ? 1 : 0);
