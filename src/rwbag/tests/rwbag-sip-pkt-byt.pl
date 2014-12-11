#! /usr/bin/perl -w
# MD5: 595788d5a684ee27601c78ad58547ae4
# TEST: ./rwbag --sip-packets=stdout --sip-bytes=/dev/null ../../tests/data.rwf | ./rwbagcat

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sip-packets=stdout --sip-bytes=/dev/null $file{data} | $rwbagcat";
my $md5 = "595788d5a684ee27601c78ad58547ae4";

check_md5_output($md5, $cmd);
