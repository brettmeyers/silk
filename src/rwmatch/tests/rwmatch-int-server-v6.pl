#! /usr/bin/perl -w
# MD5: d5181cb04def9a533b1bf173cfc9b75b
# TEST: ../rwfilter/rwfilter --daddr=2001:db8:c0:a8::/64 --dport=0-1024 --pass=stdout ../../tests/data-v6.rwf | ../rwsort/rwsort --fields=1,4,2,3,5,9 --output-path=/tmp/rwmatch-int-server-v6-incoming && ../rwfilter/rwfilter --saddr=2001:db8:c0:a8::/64 --sport=0-1024 --pass=stdout ../../tests/data-v6.rwf | ../rwsort/rwsort --fields=2,3,1,4,5,9 --output-path=/tmp/rwmatch-int-server-v6-outgoing && ./rwmatch --time-delta=2.5 --symmetric-del --relative-del --relate=1,2 --relate=4,3 --relate=2,1 --relate=3,4 --relate=5,5 /tmp/rwmatch-int-server-v6-incoming /tmp/rwmatch-int-server-v6-outgoing - | ../rwcat/rwcat --compression-method=none --byte-order=little

use strict;
use SiLKTests;

my $rwmatch = check_silk_app('rwmatch');
my $rwcat = check_silk_app('rwcat');
my $rwfilter = check_silk_app('rwfilter');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
my %temp;
$temp{incoming} = make_tempname('incoming');
$temp{outgoing} = make_tempname('outgoing');
check_features(qw(ipv6));
my $cmd = "$rwfilter --daddr=2001:db8:c0:a8::/64 --dport=0-1024 --pass=stdout $file{v6data} | $rwsort --fields=1,4,2,3,5,9 --output-path=$temp{incoming} && $rwfilter --saddr=2001:db8:c0:a8::/64 --sport=0-1024 --pass=stdout $file{v6data} | $rwsort --fields=2,3,1,4,5,9 --output-path=$temp{outgoing} && $rwmatch --time-delta=2.5 --symmetric-del --relative-del --relate=1,2 --relate=4,3 --relate=2,1 --relate=3,4 --relate=5,5 $temp{incoming} $temp{outgoing} - | $rwcat --compression-method=none --byte-order=little";
my $md5 = "d5181cb04def9a533b1bf173cfc9b75b";

check_md5_output($md5, $cmd);
