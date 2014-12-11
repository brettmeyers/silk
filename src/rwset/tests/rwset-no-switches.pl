#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwset ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwset = check_silk_app('rwset');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwset $file{empty}";

exit (check_exit_status($cmd) ? 1 : 0);
