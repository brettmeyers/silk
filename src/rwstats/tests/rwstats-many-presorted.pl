#! /usr/bin/perl -w
# MD5: 989035292cadbedf32d09bd846d27019
# TEST: ../rwfilter/rwfilter --sport=20000-25000 --pass=- ../../tests/data.rwf | ../rwsplit/rwsplit --basename=/tmp/rwstats-many-presorted-onerec --flow-limit=1 && find `dirname /tmp/rwstats-many-presorted-onerec` -type f -name `basename /tmp/rwstats-many-presorted-onerec`'*' -print | ./rwstats --fields=sport --count=70 --presorted-input --values=packets,distinct:sip,flows --xargs=-

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
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
my $cmd = "$rwfilter --sport=20000-25000 --pass=- $file{data} | $rwsplit --basename=$temp{onerec} --flow-limit=1 && find $tmpdir -type f -name '$basename*' -print | $rwstats --fields=sport --count=70 --presorted-input --values=packets,distinct:sip,flows --xargs=-";
my $md5 = "989035292cadbedf32d09bd846d27019";

check_md5_output($md5, $cmd);
