#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwidsquery --intype=rule --dry-run /tmp/rwidsquery-rule-no-date-rule 2>&1

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

my $cmd = "$rwidsquery --intype=rule --dry-run $temp{rule} 2>&1";

exit (check_exit_status($cmd) ? 1 : 0);
