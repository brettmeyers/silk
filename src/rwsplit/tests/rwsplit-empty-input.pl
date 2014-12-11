#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwsplit --basename=/tmp/rwsplit-empty-input-empty_input --flow-limit=100 ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwsplit = check_silk_app('rwsplit');
my %file;
$file{empty} = get_data_or_exit77('empty');
my %temp;
$temp{empty_input} = make_tempname('empty_input');
my $cmd = "$rwsplit --basename=$temp{empty_input} --flow-limit=100 $file{empty}";

exit (check_exit_status($cmd) ? 0 : 1);
