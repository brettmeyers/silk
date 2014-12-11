#! /usr/bin/perl -w
# MD5: 7ec34a20ac3bed8039bdd72c4f136878
# TEST: ./rwcut --fields=3-8 ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwcut --fields=3-8 $file{empty}";
my $md5 = "7ec34a20ac3bed8039bdd72c4f136878";

check_md5_output($md5, $cmd);
