#! /usr/bin/perl -w
# MD5: 83b833a44d598c1823ba55e23963eb68
# TEST: ./rwpmapcat --no-cidr ../../tests/fake-cc.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{fake_cc} = get_data_or_exit77('fake_cc');
my $cmd = "$rwpmapcat --no-cidr $file{fake_cc}";
my $md5 = "83b833a44d598c1823ba55e23963eb68";

check_md5_output($md5, $cmd);
