#! /usr/bin/perl -w
# MD5: d83c18dfe1e8cc80ca1d1d9fabe425f6
# TEST: ./rwidsquery --intype=rule --start-date=2009/02/11:10 --end-date=2009/02/11:12 --dry-run /tmp/rwidsquery-rule-rule 2>&1

use strict;
use SiLKTests;

my $rwidsquery = check_silk_app('rwidsquery');
my %temp;
$temp{rule} = make_tempname('rule');
$ENV{RWFILTER} = "rwfilter";

open SNORT, ">$temp{rule}" or exit 1;
print SNORT <<'EOF';
alert icmp $EXTERNAL_NET any -> $HOME_NET any
(msg:"ICMP Parameter Problem Bad Length"; icode:2; itype:12;
classtype:misc-activity; sid:425; rev:6;)
EOF
close SNORT or exit 1;

my $cmd = "$rwidsquery --intype=rule --start-date=2009/02/11:10 --end-date=2009/02/11:12 --dry-run $temp{rule} 2>&1";
my $md5 = "d83c18dfe1e8cc80ca1d1d9fabe425f6";

check_md5_output($md5, $cmd);
