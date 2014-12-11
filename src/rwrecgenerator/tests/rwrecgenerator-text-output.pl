#! /usr/bin/perl -w
# MD5: 0d4e979854b76697dada99ef7940df8b
# TEST: ./rwrecgenerator --seed 987654321 --log-dest=none --start-time=2011/01/01:00 --end-time=2011/01/01:01 --time-step=1000 --text-output-path -

use strict;
use SiLKTests;

my $rwrecgenerator = check_silk_app('rwrecgenerator');
my $cmd = "$rwrecgenerator --seed 987654321 --log-dest=none --start-time=2011/01/01:00 --end-time=2011/01/01:01 --time-step=1000 --text-output-path -";
my $md5 = "0d4e979854b76697dada99ef7940df8b";

check_md5_output($md5, $cmd);
