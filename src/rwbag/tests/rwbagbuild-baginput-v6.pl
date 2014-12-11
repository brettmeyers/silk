#! /usr/bin/perl -w
# MD5: 8a369187bb7df583cc306a69463b51eb
# TEST: ../rwuniq/rwuniq --fields=sip --flows --no-title ../../tests/data-v6.rwf | ./rwbagbuild --bag-input=stdin | ./rwbagcat --key-format=decimal

use strict;
use SiLKTests;

my $rwbagbuild = check_silk_app('rwbagbuild');
my $rwuniq = check_silk_app('rwuniq');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwuniq --fields=sip --flows --no-title $file{v6data} | $rwbagbuild --bag-input=stdin | $rwbagcat --key-format=decimal";
my $md5 = "8a369187bb7df583cc306a69463b51eb";

check_md5_output($md5, $cmd);
