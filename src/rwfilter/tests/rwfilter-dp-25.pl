#! /usr/bin/perl -w
# MD5: fb72798cbdb4cb24149009e51ecf9597
# TEST: ./rwfilter --dport=25 --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --dport=25 --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "fb72798cbdb4cb24149009e51ecf9597";

check_md5_output($md5, $cmd);
