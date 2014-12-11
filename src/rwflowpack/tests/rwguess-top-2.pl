#! /usr/bin/perl -w
# MD5: bc6da2ac4c7bb9e3ed07c770b6126e04
# TEST: ./rwguess --top=2 ../../tests/small.pdu

use strict;
use SiLKTests;

my $rwguess = check_silk_app('rwguess');
my %file;
$file{pdu_small} = get_data_or_exit77('pdu_small');
my $cmd = "$rwguess --top=2 $file{pdu_small}";
my $md5 = "bc6da2ac4c7bb9e3ed07c770b6126e04";

check_md5_output($md5, $cmd);
