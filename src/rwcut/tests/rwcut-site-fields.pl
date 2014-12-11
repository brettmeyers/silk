#! /usr/bin/perl -w
# MD5: 32fafd946d16021f1da5a66ef2e280ee
# TEST: ./rwcut --fields=sensor,class,type ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=sensor,class,type $file{data}";
my $md5 = "32fafd946d16021f1da5a66ef2e280ee";

check_md5_output($md5, $cmd);
