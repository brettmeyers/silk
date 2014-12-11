#! /usr/bin/perl -w
# MD5: cf143a278907d7778382c70e3151d673
# TEST: ./rwbag --sip-flows=stdout ../../tests/data-v6.rwf | ./rwbagtool --coverset | ../rwset/rwsetcat

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbag = check_silk_app('rwbag');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipset_v6));
my $cmd = "$rwbag --sip-flows=stdout $file{v6data} | $rwbagtool --coverset | $rwsetcat";
my $md5 = "cf143a278907d7778382c70e3151d673";

check_md5_output($md5, $cmd);
