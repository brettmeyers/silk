#! /usr/bin/perl -w
# MD5: d77894b456d22b7423ec18c2e2eb2d1f
# TEST: ./rwcut --fields=attributes,application ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=attributes,application $file{data}";
my $md5 = "d77894b456d22b7423ec18c2e2eb2d1f";

check_md5_output($md5, $cmd);
