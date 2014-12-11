#! /usr/bin/perl -w
# MD5: 96dfecd4ee1839441a2d7122ea2760e0
# TEST: echo 'my stdin note' | ./rwcat --note-file-add=- ../../tests/empty.rwf | ../rwfileinfo/rwfileinfo --fields=7,14 -

use strict;
use SiLKTests;

my $rwcat = check_silk_app('rwcat');
my $rwfileinfo = check_silk_app('rwfileinfo');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "echo 'my stdin note' | $rwcat --note-file-add=- $file{empty} | $rwfileinfo --fields=7,14 -";
my $md5 = "96dfecd4ee1839441a2d7122ea2760e0";

check_md5_output($md5, $cmd);
