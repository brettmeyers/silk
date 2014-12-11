#! /usr/bin/perl -w
# MD5: b3c3a864929e6bcdf415ebc9e2b5b2ce
# TEST: ./mapsid

use strict;
use SiLKTests;

my $mapsid = check_silk_app('mapsid');
my $cmd = "$mapsid";
my $md5 = "b3c3a864929e6bcdf415ebc9e2b5b2ce";

check_md5_output($md5, $cmd);
