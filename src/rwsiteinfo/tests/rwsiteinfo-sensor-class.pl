#! /usr/bin/perl -w
# MD5: 080b111ce2ab85b0c674274bd3b9148e
# TEST: ./rwsiteinfo --fields=sensor,class --site-config-file ./tests/rwsiteinfo-site.conf

use strict;
use SiLKTests;

my $rwsiteinfo = check_silk_app('rwsiteinfo');
my $cmd = "$rwsiteinfo --fields=sensor,class --site-config-file $SiLKTests::srcdir/tests/rwsiteinfo-site.conf";
my $md5 = "080b111ce2ab85b0c674274bd3b9148e";

check_md5_output($md5, $cmd);
