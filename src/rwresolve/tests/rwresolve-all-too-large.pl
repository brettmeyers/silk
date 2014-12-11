#! /usr/bin/perl -w
# MD5: 88b87ef7e5c39d0027121c09b80d1ec3
# TEST: echo '0.0.0.0|0.0.0.0|' | ./rwresolve --ip-fields=4,8 --column-width=20

use strict;
use SiLKTests;

my $rwresolve = check_silk_app('rwresolve');

exit 77 if sub { my $h = eval q{
use Socket;
my $ip = "0.0.0.0";
my $name = gethostbyaddr(inet_aton($ip),AF_INET);
return ($name ? $name : $ip); };
return (!defined $h || $h ne "0.0.0.0");
 }->();

my $cmd = "echo '0.0.0.0|0.0.0.0|' | $rwresolve --ip-fields=4,8 --column-width=20";
my $md5 = "88b87ef7e5c39d0027121c09b80d1ec3";

check_md5_output($md5, $cmd);
