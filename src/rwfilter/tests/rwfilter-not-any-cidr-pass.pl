#! /usr/bin/perl -w
# MD5: ae34ec298bda8b68b10b57bb12cf0739
# TEST: ./rwfilter --not-any-cidr=192.168.192.0/19,192.168.224.0/20,192.168.240.0/21,192.168.248.0/22,192.168.252.0/23,192.168.254.0/24,192.168.255.0/25,192.168.255.128/26,192.168.255.192/27,192.168.255.224/28,192.168.255.240/29,192.168.255.248/30,192.168.255.252/31,192.168.255.254,192.168.255.255 --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --not-any-cidr=192.168.192.0/19,192.168.224.0/20,192.168.240.0/21,192.168.248.0/22,192.168.252.0/23,192.168.254.0/24,192.168.255.0/25,192.168.255.128/26,192.168.255.192/27,192.168.255.224/28,192.168.255.240/29,192.168.255.248/30,192.168.255.252/31,192.168.255.254,192.168.255.255 --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "ae34ec298bda8b68b10b57bb12cf0739";

check_md5_output($md5, $cmd);
