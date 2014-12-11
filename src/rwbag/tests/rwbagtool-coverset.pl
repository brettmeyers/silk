#! /usr/bin/perl -w
# MD5: 3677d3da40803d98298314b69fadf06a
# TEST: ./rwbag --sip-flows=stdout ../../tests/data.rwf | ./rwbagtool --coverset | ../rwset/rwsetcat

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbag = check_silk_app('rwbag');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sip-flows=stdout $file{data} | $rwbagtool --coverset | $rwsetcat";
my $md5 = "3677d3da40803d98298314b69fadf06a";

check_md5_output($md5, $cmd);
