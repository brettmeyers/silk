#! /usr/bin/perl -w
# MD5: 94e77eeaeca3413bc7432bee0a68470b
# TEST: ./rwbagtool --compare=eq ../../tests/bag1-v6.bag ../../tests/bag3-v6.bag | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6bag1} = get_data_or_exit77('v6bag1');
$file{v6bag3} = get_data_or_exit77('v6bag3');
check_features(qw(ipv6));
my $cmd = "$rwbagtool --compare=eq $file{v6bag1} $file{v6bag3} | $rwbagcat";
my $md5 = "94e77eeaeca3413bc7432bee0a68470b";

check_md5_output($md5, $cmd);
