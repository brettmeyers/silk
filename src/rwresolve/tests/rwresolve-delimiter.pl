#! /usr/bin/perl -w
# MD5: ff3faf060e645db7d455092dce1b17a1
# TEST: echo '0.0.0.0,0.0.0.0' | ./rwresolve --delimiter=, --column-width=20

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

my $cmd = "echo '0.0.0.0,0.0.0.0' | $rwresolve --delimiter=, --column-width=20";
my $md5 = "ff3faf060e645db7d455092dce1b17a1";

check_md5_output($md5, $cmd);
