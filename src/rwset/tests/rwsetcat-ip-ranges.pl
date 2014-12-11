#! /usr/bin/perl -w
# MD5: b7ff8c82591703ee3f8671a37df5db9e
# TEST: ./rwset --sip-file=stdout ../../tests/data.rwf | ./rwsetcat --ip-ranges

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my $rwset = check_silk_app('rwset');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwset --sip-file=stdout $file{data} | $rwsetcat --ip-ranges";
my $md5 = "b7ff8c82591703ee3f8671a37df5db9e";

check_md5_output($md5, $cmd);
