#! /usr/bin/perl -w
# MD5: 9dc8a68e45e6b760566803717c24e1ba
# TEST: ./rwidsquery --intype=full --year=2009 --dry-run /tmp/rwidsquery-full-full 2>&1

use strict;
use SiLKTests;

my $rwidsquery = check_silk_app('rwidsquery');
my %temp;
$temp{full} = make_tempname('full');
$ENV{RWFILTER} = "rwfilter";

open SNORT, ">$temp{full}" or exit 1;
print SNORT <<'EOF';
[**] [1:1416:2] SNMP broadcast trap [**]
[Classification: Attempted Information Leak] [Priority: 2]
02/13-18:05:34.998533 192.168.0.1:4161 -> 10.10.10.1:139
UDP TTL:64 TOS:0x0 ID:600 IpLen:20 DgmLen:110
Len: 82
EOF
close SNORT or exit 1;

my $cmd = "$rwidsquery --intype=full --year=2009 --dry-run $temp{full} 2>&1";
my $md5 = "9dc8a68e45e6b760566803717c24e1ba";

check_md5_output($md5, $cmd);
