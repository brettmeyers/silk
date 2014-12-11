#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwdedupe ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwdedupe = check_silk_app('rwdedupe');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwdedupe $file{empty}";

exit (check_exit_status($cmd) ? 0 : 1);
