#! /usr/bin/perl -w
# MD5: ffabc5cc96bcc555e075a75982704be8
# TEST: ./rwidsquery --intype=fast --year=2009 --dry-run /tmp/rwidsquery-fast-fast 2>&1

use strict;
use SiLKTests;

my $rwidsquery = check_silk_app('rwidsquery');
my %temp;
$temp{fast} = make_tempname('fast');
$ENV{RWFILTER} = "rwfilter";

open SNORT, ">$temp{fast}" or exit 1;
print SNORT <<'EOF';
Feb  13 18:05:34 hostname snort[5214]: [1:1416:11]
SNMP broadcast trap [Classification: Attempted Information Leak]
[Priority: 2]: {TCP}
192.168.0.1:4161 -> 10.10.10.1:139
EOF
close SNORT or exit 1;

my $cmd = "$rwidsquery --intype=fast --year=2009 --dry-run $temp{fast} 2>&1";
my $md5 = "ffabc5cc96bcc555e075a75982704be8";

check_md5_output($md5, $cmd);
