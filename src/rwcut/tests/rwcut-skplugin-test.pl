#! /usr/bin/perl -w
# MD5: 595762cd9e9547952806dc1665048899
# TEST: ./rwcut --plugin=skplugin-test.so --ipv6-policy=ignore --no-columns --fields=bytes,copy-bytes,text-bytes,quant-bytes,sip,copy-sipv4,copy-sip ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
add_plugin_dirs('/src/plugins');

skip_test('Cannot load skplugin-test.so plugin')
    unless check_app_switch($rwcut.' --plugin=skplugin-test.so', 'fields', qr/copy-bytes/);
my $cmd = "$rwcut --plugin=skplugin-test.so --ipv6-policy=ignore --no-columns --fields=bytes,copy-bytes,text-bytes,quant-bytes,sip,copy-sipv4,copy-sip $file{data}";
my $md5 = "595762cd9e9547952806dc1665048899";

check_md5_output($md5, $cmd);
