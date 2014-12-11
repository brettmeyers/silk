#! /usr/bin/perl -w
# MD5: aaa697d53690f0413797c280abc4f7f2
# TEST: ../rwfilter/rwfilter --daddr=192.168.x.x --dport=0-1024 --pass=stdout ../../tests/data.rwf | ../rwsort/rwsort --fields=1,4,2,3,5,9 --output-path=/tmp/rwmatch-int-server-incoming && ../rwfilter/rwfilter --saddr=192.168.x.x --sport=0-1024 --pass=stdout ../../tests/data.rwf | ../rwsort/rwsort --fields=2,3,1,4,5,9 --output-path=/tmp/rwmatch-int-server-outgoing && ./rwmatch --ipv6-policy=asv4 --time-delta=2.5 --symmetric-del --relative-del --relate=1,2 --relate=4,3 --relate=2,1 --relate=3,4 --relate=5,5 /tmp/rwmatch-int-server-incoming /tmp/rwmatch-int-server-outgoing - | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwmatch = check_silk_app('rwmatch');
my $rwcat = check_silk_app('rwcat');
my $rwfilter = check_silk_app('rwfilter');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
my %temp;
$temp{incoming} = make_tempname('incoming');
$temp{outgoing} = make_tempname('outgoing');
my $cmd = "$rwfilter --daddr=192.168.x.x --dport=0-1024 --pass=stdout $file{data} | $rwsort --fields=1,4,2,3,5,9 --output-path=$temp{incoming} && $rwfilter --saddr=192.168.x.x --sport=0-1024 --pass=stdout $file{data} | $rwsort --fields=2,3,1,4,5,9 --output-path=$temp{outgoing} && $rwmatch --ipv6-policy=asv4 --time-delta=2.5 --symmetric-del --relative-del --relate=1,2 --relate=4,3 --relate=2,1 --relate=3,4 --relate=5,5 $temp{incoming} $temp{outgoing} - | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "aaa697d53690f0413797c280abc4f7f2";

check_md5_output($md5, $cmd);
