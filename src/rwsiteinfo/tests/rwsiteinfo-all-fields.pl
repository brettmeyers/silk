#! /usr/bin/perl -w
# MD5: 46aabbb77d0764b41c4fd14c2b952737
# TEST: ./rwsiteinfo --fields=class,type,flowtype,id-flowtype,sensor,id-sensor,describe-sensor,default-class,default-type,mark-defaults,class:list,type:list,flowtype:list,id-flowtype:list,sensor:list,id-sensor:list,default-class:list,default-type:list --site-config-file ./tests/rwsiteinfo-site.conf

use strict;
use SiLKTests;

my $rwsiteinfo = check_silk_app('rwsiteinfo');
my $cmd = "$rwsiteinfo --fields=class,type,flowtype,id-flowtype,sensor,id-sensor,describe-sensor,default-class,default-type,mark-defaults,class:list,type:list,flowtype:list,id-flowtype:list,sensor:list,id-sensor:list,default-class:list,default-type:list --site-config-file $SiLKTests::srcdir/tests/rwsiteinfo-site.conf";
my $md5 = "46aabbb77d0764b41c4fd14c2b952737";

check_md5_output($md5, $cmd);
