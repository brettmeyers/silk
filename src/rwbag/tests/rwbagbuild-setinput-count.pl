#! /usr/bin/perl -w
# MD5: 45e47b334ef7f46bde87a39f8767b38a
# TEST: ../rwset/rwset --sip-file=stdout ../../tests/data.rwf | ./rwbagbuild --set-input=stdin --default-count=200 | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagbuild = check_silk_app('rwbagbuild');
my $rwset = check_silk_app('rwset');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwset --sip-file=stdout $file{data} | $rwbagbuild --set-input=stdin --default-count=200 | $rwbagcat";
my $md5 = "45e47b334ef7f46bde87a39f8767b38a";

check_md5_output($md5, $cmd);
