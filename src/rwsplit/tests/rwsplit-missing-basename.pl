#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwsplit --flow-limit=100 ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwsplit = check_silk_app('rwsplit');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwsplit --flow-limit=100 $file{empty}";

exit (check_exit_status($cmd) ? 1 : 0);
