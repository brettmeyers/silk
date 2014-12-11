#! /usr/bin/perl -w
# MD5: 34fd696cef3ab038af6b4edb6ed376ef
# TEST: ../rwcut/rwcut --fields=sip --ipv6-policy=ignore --no-title --delimited ../../tests/data.rwf | ./rwip2cc --input-file=-

use strict;
use SiLKTests;

my $rwip2cc = check_silk_app('rwip2cc');
my $rwcut = check_silk_app('rwcut');
my %file;
$file{fake_cc} = get_data_or_exit77('fake_cc');
$file{data} = get_data_or_exit77('data');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
my $cmd = "$rwcut --fields=sip --ipv6-policy=ignore --no-title --delimited $file{data} | $rwip2cc --input-file=-";
my $md5 = "34fd696cef3ab038af6b4edb6ed376ef";

check_md5_output($md5, $cmd);
