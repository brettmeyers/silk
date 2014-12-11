#! /usr/bin/perl -w
# MD5: 2c62dd48ddcdf43ede20d5b17fc631ca
# TEST: ./rwsettool --intersect ../../tests/set3-v4.set ../../tests/set4-v4.set  > /tmp/rwsettool-symmet-diff-s3-s4-v4-intersect  && ./rwsettool --union ../../tests/set3-v4.set ../../tests/set4-v4.set | ./rwsettool --difference - /tmp/rwsettool-symmet-diff-s3-s4-v4-intersect | ./rwsetcat --cidr

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set3} = get_data_or_exit77('v4set3');
$file{v4set4} = get_data_or_exit77('v4set4');
my %temp;
$temp{intersect} = make_tempname('intersect');
my $cmd = "$rwsettool --intersect $file{v4set3} $file{v4set4}  > $temp{intersect}  && $rwsettool --union $file{v4set3} $file{v4set4} | $rwsettool --difference - $temp{intersect} | $rwsetcat --cidr";
my $md5 = "2c62dd48ddcdf43ede20d5b17fc631ca";

check_md5_output($md5, $cmd);
