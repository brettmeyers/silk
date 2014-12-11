#! /usr/bin/perl -w
# MD5: 393789257810fde6263977f90d106343
# TEST: ../rwcut/rwcut --fields=sip,dip,sport,dport,proto,packets,bytes,stime,dur,sensor,class,type,in,out,application,initialflags,sessionflags,attributes ../../tests/data.rwf | ./rwtuc | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwtuc = check_silk_app('rwtuc');
my $rwcut = check_silk_app('rwcut');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=sip,dip,sport,dport,proto,packets,bytes,stime,dur,sensor,class,type,in,out,application,initialflags,sessionflags,attributes $file{data} | $rwtuc | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "393789257810fde6263977f90d106343";

check_md5_output($md5, $cmd);
