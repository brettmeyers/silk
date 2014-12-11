#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwcompare ../../tests/data.rwf /dev/null

use strict;
use SiLKTests;

my $rwcompare = check_silk_app('rwcompare');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcompare $file{data} /dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
