#! /usr/bin/perl -w
# MD5: 5360ad5a52678d4936e5a83822e86b1a
# TEST: ./rwfilter --proto=17 --print-volume-statistics=stdout ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{data} = get_data_or_exit77('data');
add_plugin_dirs('/src/rwipa');

skip_test('Cannot load IPA')
    unless check_app_switch($rwfilter, 'ipa-src-expr');
my $cmd = "$rwfilter --proto=17 --print-volume-statistics=stdout $file{data}";
my $md5 = "5360ad5a52678d4936e5a83822e86b1a";

check_md5_output($md5, $cmd);
