#! /usr/bin/perl -w
# MD5: 318c4ea2128a1d2bdc45a98208dd773c
# TEST: cat /dev/null | ./rwuniq --fields=sport --presorted-input -xargs=-

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my $cmd = "cat /dev/null | $rwuniq --fields=sport --presorted-input -xargs=-";
my $md5 = "318c4ea2128a1d2bdc45a98208dd773c";

check_md5_output($md5, $cmd);
