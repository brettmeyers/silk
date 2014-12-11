#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ../rwset/rwset --sip=- ../../tests/empty.rwf | ./rwipaimport --catalog=my-cat --description=my-description --start-time=2009/02/12:00:00 --end-time=2009/02/14:23:59:59 -

use strict;
use SiLKTests;

my $rwipaimport = check_silk_app('rwipaimport');
my $rwset = check_silk_app('rwset');
my %file;
$file{empty} = get_data_or_exit77('empty');
check_features(qw(ipa));
my $cmd = "$rwset --sip=- $file{empty} | $rwipaimport --catalog=my-cat --description=my-description --start-time=2009/02/12:00:00 --end-time=2009/02/14:23:59:59 -";

exit (check_exit_status($cmd) ? 1 : 0);
