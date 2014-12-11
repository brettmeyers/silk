#! /usr/bin/perl -w
# MD5: 3677d3da40803d98298314b69fadf06a
# TEST: ./rwaddrcount --set-file=stdout ../../tests/data.rwf | ../rwset/rwsetcat

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwaddrcount --set-file=stdout $file{data} | $rwsetcat";
my $md5 = "3677d3da40803d98298314b69fadf06a";

check_md5_output($md5, $cmd);
