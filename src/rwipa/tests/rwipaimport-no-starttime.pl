#! /usr/bin/perl -w
# ERR_MD5: 93700da53ada812efacf32df65d09ddd
# TEST: ../rwset/rwset --sip=- ../../tests/empty.rwf | ./rwipaimport --catalog=my-cat --description=my-description --end-time=2009/02/14:23:59:59 - 2>&1

use strict;
use SiLKTests;

my $rwipaimport = check_silk_app('rwipaimport');
my $rwset = check_silk_app('rwset');
my %file;
$file{empty} = get_data_or_exit77('empty');
check_features(qw(ipa));
my $cmd = "$rwset --sip=- $file{empty} | $rwipaimport --catalog=my-cat --description=my-description --end-time=2009/02/14:23:59:59 - 2>&1";
my $md5 = "93700da53ada812efacf32df65d09ddd";

check_md5_output($md5, $cmd, 1);
