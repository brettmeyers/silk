#! /usr/bin/perl -w
# MD5: e6380cf990338c93e1a6f289bef4bd72
# TEST: ./rwsiteinfo --delimited='+' --fields=class,type --site-config-file ./tests/rwsiteinfo-site.conf

use strict;
use SiLKTests;

my $rwsiteinfo = check_silk_app('rwsiteinfo');
my $cmd = "$rwsiteinfo --delimited='+' --fields=class,type --site-config-file $SiLKTests::srcdir/tests/rwsiteinfo-site.conf";
my $md5 = "e6380cf990338c93e1a6f289bef4bd72";

check_md5_output($md5, $cmd);
