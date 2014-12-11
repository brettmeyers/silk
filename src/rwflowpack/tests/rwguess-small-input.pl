#! /usr/bin/perl -w
# MD5: fd3bbe44652379583d202c375f48cff9
# TEST: ./rwguess ../../tests/small.pdu

use strict;
use SiLKTests;

my $rwguess = check_silk_app('rwguess');
my %file;
$file{pdu_small} = get_data_or_exit77('pdu_small');
my $cmd = "$rwguess $file{pdu_small}";
my $md5 = "fd3bbe44652379583d202c375f48cff9";

check_md5_output($md5, $cmd);
