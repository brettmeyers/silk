#! /usr/bin/perl -w
# MD5: bd94741f304044e69d2cbeb15eccc2c7
# TEST: ../rwset/rwset --sip-file=stdout ../../tests/data.rwf | ./rwbagbuild --set-input=stdin --output-path=stdout | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagbuild = check_silk_app('rwbagbuild');
my $rwset = check_silk_app('rwset');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwset --sip-file=stdout $file{data} | $rwbagbuild --set-input=stdin --output-path=stdout | $rwbagcat";
my $md5 = "bd94741f304044e69d2cbeb15eccc2c7";

check_md5_output($md5, $cmd);
