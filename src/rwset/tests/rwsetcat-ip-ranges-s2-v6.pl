#! /usr/bin/perl -w
# MD5: 323136572641a9994dc34128ccda9cfb
# TEST: ./rwsetcat --ip-ranges --ip-format=zero-padded ../../tests/set2-v6.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set2} = get_data_or_exit77('v6set2');
check_features(qw(ipset_v6));
my $cmd = "$rwsetcat --ip-ranges --ip-format=zero-padded $file{v6set2}";
my $md5 = "323136572641a9994dc34128ccda9cfb";

check_md5_output($md5, $cmd);
