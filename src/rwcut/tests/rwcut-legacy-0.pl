#! /usr/bin/perl -w
# MD5: c72718d93ca77eef65f9bb5fdba682a2
# TEST: ./rwcut --fields=9,11 --timestamp-format=default ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=9,11 --timestamp-format=default $file{data}";
my $md5 = "c72718d93ca77eef65f9bb5fdba682a2";

check_md5_output($md5, $cmd);
