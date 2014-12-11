#! /usr/bin/perl -w
# MD5: f4ceb1b02d2db92f50e3721a940d2e3d
# TEST: ./rwpdu2silk ../../tests/small.pdu | ../rwcat/rwcat --byte-order=big --ipv4-output --compression=none

use strict;
use SiLKTests;

my $rwpdu2silk = check_silk_app('rwpdu2silk');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{pdu_small} = get_data_or_exit77('pdu_small');
my $cmd = "$rwpdu2silk $file{pdu_small} | $rwcat --byte-order=big --ipv4-output --compression=none";
my $md5 = "f4ceb1b02d2db92f50e3721a940d2e3d";

check_md5_output($md5, $cmd);
