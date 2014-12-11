#! /usr/bin/perl -w
# MD5: 4570cf3f6a419c5f44fe38976cadcac5
# TEST: ../rwfilter/rwfilter --sport=20000-25000 --pass=- ../../tests/data.rwf | ../rwsplit/rwsplit --basename=/tmp/rwsort-many-presorted-onerec --flow-limit=1 && find `dirname /tmp/rwsort-many-presorted-onerec` -type f -name `basename /tmp/rwsort-many-presorted-onerec`'*' -print | ./rwsort --fields=sport --presorted-input --xargs=- | ../rwcut/rwcut --fields=sport

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwcut = check_silk_app('rwcut');
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
my $cmd = "$rwfilter --sport=20000-25000 --pass=- $file{data} | $rwsplit --basename=$temp{onerec} --flow-limit=1 && find $tmpdir -type f -name '$basename*' -print | $rwsort --fields=sport --presorted-input --xargs=- | $rwcut --fields=sport";
my $md5 = "4570cf3f6a419c5f44fe38976cadcac5";

check_md5_output($md5, $cmd);
