#! /usr/bin/perl -w
# MD5: b1574040455b0d48517bd6ee18e0423a
# TEST: ./rwcat --note-add='my command line note' ../../tests/empty.rwf | ../rwfileinfo/rwfileinfo --fields=7,14 -

use strict;
use SiLKTests;

my $rwcat = check_silk_app('rwcat');
my $rwfileinfo = check_silk_app('rwfileinfo');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwcat --note-add='my command line note' $file{empty} | $rwfileinfo --fields=7,14 -";
my $md5 = "b1574040455b0d48517bd6ee18e0423a";

check_md5_output($md5, $cmd);
