#! /usr/bin/perl -w
# MD5: multiple
# TEST: ./rwsplit --basename=$temp --ip-limit=5000 ../../tests/data.rwf && ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output $temp*

use strict;
use SiLKTests;

my $rwsplit = check_silk_app('rwsplit');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $temp = make_tempname('ip-5k');
my $cmd = "$rwsplit --basename=$temp --ip-limit=5000 $file{data}";

# clean up when we're done
END {
    if (!$ENV{SK_TESTS_SAVEOUTPUT}) {
        # remove files
        unlink glob($temp."*");
    }
}

if (!check_exit_status($cmd)) {
    exit 1;
}

# compute MD5 of each file
for (my $i = 0; my $md5 = <DATA>; ++$i) {
    chomp $md5;
    my $f = sprintf("%s.%08d.rwf", $temp, $i);
    $cmd = "$rwcat --compression-method=none --byte-order=little --ipv4-output $f";
    check_md5_output($md5, $cmd);
}

__DATA__
e3859c34f1034911261b615a1aac3854
feb8e865c5b0e9947b0c90d33079af47
5401ce83b96cc752936452d6b14a75cd
82eec9477727c074737eb5cc49472cb0
2d70b239d5db50984eedce6a1e861f12
eac219731ee7be7c402fabd2664591f0
0bb43b4a656208bea1ba7cb0437cfbb8
abc4a98c8e5b79cc27e99e0012e0f72d
de370b818146f65b07147dc4b16091d1
fd00b36d5a52e756ef8182b24decfe80
f8162acf725fc992216cd4c96a36beb6
ccd19b0310bdac03e324c3c59c173b0a
acc20cb9255b1dd9e9702cbf64316529
f9bc1be61d8c0a5092428c542a829363
e1610581f83d61c7eb22c2de45063007
1950ba855f4f2e01804b064fc11b3fa2
ebb61b683c3bf142b327c039f6ee156f
c749d0994ddd8be760d34471bcb3a361
c85653a121b1551a989ee79b4b6a577a
58886f5aced1a56f336926cbc3003056
024932a5102a13d86f3fdffe85460abb
7cbc32103407a1331b5277e3f1fb5a95
26a61b4a079c0db7094947bdff67e283
475a124c65f58c2b695e4caa69cd9b53
938165064631ea31e041a2110c1dcb18
f89129daddfc8a75cfef2ae8e801ed01
2a09d1699303e5231b1999802966d313
f5ed2929a6c64729f799c5582d2a1211
0212ff2ed9e457845bd3f079121aa2ce
29351c95b4cd5721485038236bc239a7
74e9de6ccd5850d109afc4cf53d6a59d
b65862d7a123f4132061be59cf9af5b5
b5b3358b0783de731624eac64ec21f38
432aebefe646d25cfcdc6a33ea84c7cc
6e58afab54ea3af0c884a80695e717c0
fc4015ea866001faf8a549abef0feee4
57b71c36a9499d4ecab7dfdb9519556a
c705d608e70744c608dadc28d5cdef4f
871595c300d0c91da0b345b355f9d761
77c2a4c2a4818c0eb122f9e9b59915d6
5c9f4eaf8fca3e1e0bd596233389d5a4
9e27dfa3b3080c9355e4885e6c5f5045
868e0ad00adb00654a3f1c50fe8e0733
7796da169412f6de4bff541b0541384a
014725a864c3825a781dc3dc16d80bf2
318c27a4b1fd8a82e8d7c75156f1a47b
50f27fc23ca2decfd2b8a782d7ac3441
93d7da3d87f23532b732b9f0237a4e76
16c9b029df78ec063240ac23ae3110c9
f6384e9e4ff262b30cc8598de9ef6409
0632ee9454e81dabba83872a3633fdd6
bb0b22e45cdcdd4bbcb022aced9fafce
30e71a57ffbc3b5e858396e84bca940f
7fca34c1e0e7a5178b9d8a8d9ce89e68
78e2b4ceb6d138dca9d584383cda5b69
1fd0de9febe971893e6b315342f27a4e
f8a1a4fa8fddd9f8314c3697c0776510
3a6a4ab4b7fe8be924ff3cdff4d3bc34
21e1d9d29f87a96c8c51ea15de9c9008
bccc16bb68ee17ca34bbee1261d1d0b3
0b1479907711a57689b47aa746f5fc48
abe302f2f3dd250a37f6a2844d9e865a
3d632fcd664d69d284e1f3e906a9b15a
