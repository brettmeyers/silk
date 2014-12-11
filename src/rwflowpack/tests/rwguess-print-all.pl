#! /usr/bin/perl -w
# MD5: 62f82312c2c91c7418df89380568e3c1
# TEST: ./rwguess --print-all ../../tests/small.pdu

use strict;
use SiLKTests;

my $rwguess = check_silk_app('rwguess');
my %file;
$file{pdu_small} = get_data_or_exit77('pdu_small');
my $cmd = "$rwguess --print-all $file{pdu_small}";
my $md5 = "62f82312c2c91c7418df89380568e3c1";

check_md5_output($md5, $cmd);
