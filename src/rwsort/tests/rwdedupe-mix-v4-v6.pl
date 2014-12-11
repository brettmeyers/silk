#! /usr/bin/perl -w
# MD5: 71dc8840c8ae3538597504a628e3b1df
# TEST: ../rwtuc/rwtuc --fields=sip /tmp/ips | ./rwdedupe | ../rwcut/rwcut --fields=sip --no-title --delimited

use strict;
use SiLKTests;

my $NAME = $0;
$NAME =~ s,.*/,,;

my $rwdedupe = check_silk_app('rwdedupe');
my $rwcut = check_silk_app('rwcut');
my $rwtuc = check_silk_app('rwtuc');
my %file;
check_features(qw(ipv6));

my %temp;
$temp{ips} = make_tempname('ips');

open IPS, '>', $temp{ips}
    or die "$NAME: Cannot open '$temp{ips}': $!\n";
print IPS <<'EOF';
2001:db8::5
::1
10.0.0.2
2001:db8::6
::ffff:10.0.0.2
EOF

my $cmd = ("$rwtuc --fields=sip $temp{ips}"
           ." | $rwdedupe"
           ." | $rwcut --fields=sip --no-title --delimited");
my $md5 = "71dc8840c8ae3538597504a628e3b1df";

check_md5_output($md5, $cmd);
