#! /usr/bin/perl -w
# MD5: 76285b5908efdf205de125b2003090fb
# TEST: ./rwcut --fields=7,6 --column-separator=/ ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=7,6 --column-separator=/ $file{data}";
my $md5 = "76285b5908efdf205de125b2003090fb";

check_md5_output($md5, $cmd);
