#! /usr/bin/perl -w
# MD5: 06898de2a61b8470ffb9267e5231e19a
# TEST: ../rwuniq/rwuniq --fields=sport --flows --no-title ../../tests/data.rwf | ./rwbagbuild --bag-input=stdin | ./rwbagcat --key-format=decimal

use strict;
use SiLKTests;

my $rwbagbuild = check_silk_app('rwbagbuild');
my $rwuniq = check_silk_app('rwuniq');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=sport --flows --no-title $file{data} | $rwbagbuild --bag-input=stdin | $rwbagcat --key-format=decimal";
my $md5 = "06898de2a61b8470ffb9267e5231e19a";

check_md5_output($md5, $cmd);
