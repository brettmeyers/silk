#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwsort ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwsort $file{empty}";

exit (check_exit_status($cmd) ? 1 : 0);
