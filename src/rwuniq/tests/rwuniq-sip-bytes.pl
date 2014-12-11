#! /usr/bin/perl -w
# MD5: 39884aedf3ed40f6220f9044f3c0488f
# TEST: ./rwuniq --fields=sip --bytes --ipv6-policy=ignore --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=sip --bytes --ipv6-policy=ignore --sort-output $file{data}";
my $md5 = "39884aedf3ed40f6220f9044f3c0488f";

check_md5_output($md5, $cmd);
