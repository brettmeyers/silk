#! /usr/bin/perl -w
# MD5: 27450d7fbb20c3cfd6a3c59df800b125
# TEST: echo 65536,100 | ./rwbagbuild --bag-input=stdin --delimiter=, --key-type=dport --counter-type=sum-bytes | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagbuild = check_silk_app('rwbagbuild');
my $rwbagcat = check_silk_app('rwbagcat');
my $cmd = "echo 65536,100 | $rwbagbuild --bag-input=stdin --delimiter=, --key-type=dport --counter-type=sum-bytes | $rwbagcat";
my $md5 = "27450d7fbb20c3cfd6a3c59df800b125";

check_md5_output($md5, $cmd);
