#! /usr/bin/perl -w
# MD5: d8e62920a24d4c799816fec3b51d8ca1
# TEST: ../rwset/rwset --sip-file=stdout ../../tests/data-v6.rwf | ./rwbagbuild --set-input=stdin | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagbuild = check_silk_app('rwbagbuild');
my $rwset = check_silk_app('rwset');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipset_v6));
my $cmd = "$rwset --sip-file=stdout $file{v6data} | $rwbagbuild --set-input=stdin | $rwbagcat";
my $md5 = "d8e62920a24d4c799816fec3b51d8ca1";

check_md5_output($md5, $cmd);
