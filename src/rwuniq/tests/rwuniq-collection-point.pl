#! /usr/bin/perl -w
# MD5: 9c75b830f20c5de33e22ef7df5e3884d
# TEST: ./rwuniq --fields=sensor,class,type --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=sensor,class,type --sort-output $file{data}";
my $md5 = "9c75b830f20c5de33e22ef7df5e3884d";

check_md5_output($md5, $cmd);
