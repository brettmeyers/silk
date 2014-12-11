#! /usr/bin/perl -w
# MD5: 85d36a51f3b44bab244646145464cc41
# TEST: ./rwsiteinfo --fields=class,default-class --site-config-file ./tests/rwsiteinfo-site.conf

use strict;
use SiLKTests;

my $rwsiteinfo = check_silk_app('rwsiteinfo');
my $cmd = "$rwsiteinfo --fields=class,default-class --site-config-file $SiLKTests::srcdir/tests/rwsiteinfo-site.conf";
my $md5 = "85d36a51f3b44bab244646145464cc41";

check_md5_output($md5, $cmd);
