#! /usr/bin/perl -w
# ERR_MD5: b61de1d04d0ef762207b0ff348f5b900
# TEST: echo 172.16-31.x.x | ../rwset/rwsetbuild - - | ./rwpackchecker --value match-sip=- ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwpackchecker = check_silk_app('rwpackchecker');
my $rwsetbuild = check_silk_app('rwsetbuild');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "echo 172.16-31.x.x | $rwsetbuild - - | $rwpackchecker --value match-sip=- $file{data}";
my $md5 = "b61de1d04d0ef762207b0ff348f5b900";

check_md5_output($md5, $cmd, 1);
