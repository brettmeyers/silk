#! /usr/bin/perl -w
# MD5: 3bb69ffad0d039a4ded5e6ffe046fb77
# TEST: ./rwstats --fields=proto --values=sip-distinct,dip-distinct --count=5 --ipv6=ignore --no-percent ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=proto --values=sip-distinct,dip-distinct --count=5 --ipv6=ignore --no-percent $file{data}";
my $md5 = "3bb69ffad0d039a4ded5e6ffe046fb77";

check_md5_output($md5, $cmd);
