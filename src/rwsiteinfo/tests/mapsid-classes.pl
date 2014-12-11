#! /usr/bin/perl -w
# MD5: 7aef9f24423fe075091ecbff3e2e940e
# TEST: ./mapsid --print-classes

use strict;
use SiLKTests;

my $mapsid = check_silk_app('mapsid');
my $cmd = "$mapsid --print-classes";
my $md5 = "7aef9f24423fe075091ecbff3e2e940e";

check_md5_output($md5, $cmd);
