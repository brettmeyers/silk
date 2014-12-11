#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwsplit --basename=/tmp/rwsplit-null-input-null_input --flow-limit=100 /dev/null

use strict;
use SiLKTests;

my $rwsplit = check_silk_app('rwsplit');
my %temp;
$temp{null_input} = make_tempname('null_input');
my $cmd = "$rwsplit --basename=$temp{null_input} --flow-limit=100 /dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
