#! /usr/bin/perl -w
# MD5: 57b29617a20c3690f7ff546859f2d796
# TEST: ./rwset --sip-file=stdout ../../tests/data.rwf | ./rwsetcat --network-structure=ATS

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my $rwset = check_silk_app('rwset');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwset --sip-file=stdout $file{data} | $rwsetcat --network-structure=ATS";
my $md5 = "57b29617a20c3690f7ff546859f2d796";

check_md5_output($md5, $cmd);
