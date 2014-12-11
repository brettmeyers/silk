#! /usr/bin/perl -w
# MD5: 50f4079aed63ecf52ce544953a3b2953
# TEST: ./rwcat ../../tests/data-v6.rwf | ../rwcut/rwcut --fields=1-15,20,21,26-29 --timestamp-format=epoch --delimited

use strict;
use SiLKTests;

my $rwcat = check_silk_app('rwcat');
my $rwcut = check_silk_app('rwcut');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwcat $file{v6data} | $rwcut --fields=1-15,20,21,26-29 --timestamp-format=epoch --delimited";
my $md5 = "50f4079aed63ecf52ce544953a3b2953";

check_md5_output($md5, $cmd);
