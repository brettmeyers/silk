#! /usr/bin/perl -w
# MD5: d0e6ae79dd2ebc04325ad7c04b6ece38
# TEST: ../rwfilter/rwfilter --sport=20000-25000 --pass=- ../../tests/data.rwf | ../rwsplit/rwsplit --basename=/tmp/rwuniq-many-presorted-onerec --flow-limit=1 && find `dirname /tmp/rwuniq-many-presorted-onerec`  -type f -name `basename /tmp/rwuniq-many-presorted-onerec`'*' -print | ./rwuniq --fields=sport --values=packets,flows,distinct:sip --presorted-input --xargs=-

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my $rwfilter = check_silk_app('rwfilter');
my $rwsplit = check_silk_app('rwsplit');
my %file;
$file{data} = get_data_or_exit77('data');
my %temp;
$temp{onerec} = make_tempname('onerec');
my $tmpdir = $temp{onerec};
$tmpdir =~ s,^(.*/).*,$1,;
my $basename = $temp{onerec};
$basename =~ s,^.*/(.+),$1,;
my $cmd = "$rwfilter --sport=20000-25000 --pass=- $file{data} | $rwsplit --basename=$temp{onerec} --flow-limit=1 && find $tmpdir -type f -name '$basename*' -print | $rwuniq --fields=sport --values=packets,flows,distinct:sip --presorted-input --xargs=-";
my $md5 = "d0e6ae79dd2ebc04325ad7c04b6ece38";

check_md5_output($md5, $cmd);
