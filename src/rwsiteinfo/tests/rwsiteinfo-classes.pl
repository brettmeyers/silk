#! /usr/bin/perl -w
# MD5: 9c827eab26ddce21d4743b27fc1fdfc4
# TEST: ./rwsiteinfo --fields=class --site-config-file ./tests/rwsiteinfo-site.conf

use strict;
use SiLKTests;

my $rwsiteinfo = check_silk_app('rwsiteinfo');
my $cmd = "$rwsiteinfo --fields=class --site-config-file $SiLKTests::srcdir/tests/rwsiteinfo-site.conf";
my $md5 = "9c827eab26ddce21d4743b27fc1fdfc4";

check_md5_output($md5, $cmd);
