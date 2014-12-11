#! /usr/bin/perl -w
# MD5: 0e0c59c76fee0e52acc7b01ffa22ffdf
# TEST: ./rwsettool --intersect ../../tests/set4-v6.set ../../tests/set3-v6.set  > /tmp/rwsettool-symmet-diff-s4-s3-v6-intersect  && ./rwsettool --union ../../tests/set4-v6.set ../../tests/set3-v6.set | ./rwsettool --difference - /tmp/rwsettool-symmet-diff-s4-s3-v6-intersect | ./rwsetcat --cidr

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set3} = get_data_or_exit77('v6set3');
$file{v6set4} = get_data_or_exit77('v6set4');
my %temp;
$temp{intersect} = make_tempname('intersect');
my $cmd = "$rwsettool --intersect $file{v6set4} $file{v6set3}  > $temp{intersect}  && $rwsettool --union $file{v6set4} $file{v6set3} | $rwsettool --difference - $temp{intersect} | $rwsetcat --cidr";
my $md5 = "0e0c59c76fee0e52acc7b01ffa22ffdf";

check_md5_output($md5, $cmd);
