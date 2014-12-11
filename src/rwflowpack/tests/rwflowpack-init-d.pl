#! /usr/bin/perl -w
#
#
# RCSIDENT("$SiLK: rwflowpack-init-d.pl 7db00ddfdd25 2014-05-30 18:58:37Z mthomas $")

use strict;
use SiLKTests;

my $NAME = $0;
$NAME =~ s,.*/,,;

# set the environment variables required for rwflowpack to find its
# packing logic plug-in
add_plugin_dirs('/site/twoway');

# Skip this test if we cannot load the packing logic
check_exit_status("./rwflowpack --sensor-conf=$srcdir/tests/sensor77.conf"
                  ." --verify-sensor-conf")
    or skip_test("Cannot load packing logic");

# create our tempdir
my $tmpdir = make_tempdir();

# the daemon being tested and the DAEMON.init.d and DAEMON.conf files
my $DAEMON = 'rwflowpack';

my $daemon_init = "$DAEMON.init.d";
unless (-x $daemon_init) {
    skip_test("Missing start-up script '$daemon_init'");
}
check_daemon_init_program_name($daemon_init, $DAEMON);

my $daemon_src = "$DAEMON.conf";
unless (-f $daemon_src) {
    skip_test("Missing template file '$daemon_src'");
}

my $daemon_conf = "$tmpdir/$DAEMON.conf";

# set environment variable to the directory holding $DAEMON.conf
$ENV{SCRIPT_CONFIG_LOCATION} = $tmpdir;


# directories
my $log_dir = "$tmpdir/log";
my $data_dir = "$tmpdir/data";
my $netflow_dir = "$tmpdir/netflow";

# create the data directory and copy the silk.conf file
if (-f $ENV{SILK_CONFIG_FILE}) {
    mkdir $data_dir, 0700
        or die "$NAME: Unable to create directory '$data_dir': $!\n";
    system "cp", $ENV{SILK_CONFIG_FILE}, "$data_dir/silk.conf";
}

# create an incoming directory for netflow-v5 files
mkdir $netflow_dir, 0700
    or die "$NAME: Unable to create directory '$netflow_dir': $!\n";

# receive data to this port and host
my $host = '127.0.0.1';
my $port = get_ephemeral_port($host, 'udp');

# create the sensor.conf
my $sensor_conf = "$tmpdir/sensor.conf";
open SENSOR_OUT, ">$sensor_conf"
    or die "$NAME: Cannot open '$sensor_conf': $!\n";
print SENSOR_OUT <<EOF;
probe S0 netflow-v5
    protocol udp
    listen-on-port $port
    listen-as-host $host
end probe

sensor S0
    netflow-v5-probes S0
    source-network external
    destination-network internal
end sensor

probe S1 netflow-v5
    poll-directory $netflow_dir
end probe

sensor S1
    netflow-v5-probes S1
    source-network internal
    destination-network external
end sensor
EOF
close SENSOR_OUT
    or die "$NAME: Cannot close '$sensor_conf': $!\n";

# open the template file for $DAEMON.conf
open SRC, $daemon_src
    or die "$NAME: Cannot open template file '$daemon_src': $!\n";

# create $DAEMON.conf
open CONF, ">$daemon_conf"
    or die "$NAME: Cannot create configuration file '$daemon_conf': $!\n";
while (<SRC>) {
    chomp;
    s/\#.*//;
    next unless /\S/;

    if (/^(BIN_DIR=).*/) {
        my $pwd = `pwd`;
        print CONF $1, $pwd;
        next;
    }
    if (/^(CREATE_DIRECTORIES=).*/) {
        print CONF $1, "yes\n";
        next;
    }
    if (/^(ENABLED=).*/) {
        print CONF $1, "yes\n";
        next;
    }
    if (/^(LOG_TYPE=).*/) {
        print CONF $1, "legacy\n";
        next;
    }
    if (/^(statedirectory=).*/) {
        print CONF $1, $tmpdir, "\n";
        next;
    }
    if (/^(USER=).*/) {
        print CONF $1, '`whoami`', "\n";
        next;
    }

    if (/^(DATA_ROOTDIR=).*/) {
        print CONF $1, $data_dir, "\n";
        next;
    }
    if (/^(SENSOR_CONFIG=).*/) {
        print CONF $1, $sensor_conf, "\n";
        next;
    }
    if (/^(FLAT_ARCHIVE=).*/) {
        print CONF $1, "1\n";
        next;
    }

    print CONF $_,"\n";
}
close CONF
    or die "$NAME: Cannot close '$daemon_conf': $!\n";
close SRC;


my $cmd;
my $expected_status;
my $good = 1;
my $pid;

# run "daemon.init.d status"; it should not be running
$expected_status = 'stopped';
$cmd = "./$daemon_init status";
run_command($cmd, \&init_d);
exit 1
    unless $good;

# run "daemon.init.d start"
$expected_status = 'starting';
$cmd = "./$daemon_init start";
run_command($cmd, \&init_d);

# get the PID from the pidfile
my $pidfile = "$log_dir/$DAEMON.pid";
if (-f $pidfile) {
    open PID, $pidfile
        or die "$NAME: Cannot open '$pidfile': $!\n";
    $pid = "";
    while (my $x = <PID>) {
        $pid .= $x;
    }
    close PID;
    if ($pid && $pid =~ /^(\d+)/) {
        $pid = $1;
    }
    else {
        $pid = undef;
    }
}

# kill the process if the daemon.init.d script failed
unless ($good) {
    if ($pid) {
        kill_process($pid);
    }
    exit 1;
}

sleep 2;

# run "daemon.init.d stutus"; it should be running
$expected_status = 'running';
$cmd = "./$daemon_init status";
run_command($cmd, \&init_d);

sleep 2;

$expected_status = 'stopping';
$cmd = "./$daemon_init stop";
run_command($cmd, \&init_d);

sleep 2;

# run "daemon.init.d status"; it should not be running
$expected_status = 'stopped';
$cmd = "./$daemon_init status";
run_command($cmd, \&init_d);

if ($pid) {
    if (kill 0, $pid) {
        print STDERR "$NAME: Process $pid is still running\n";
        $good = 0;
        kill_process($pid);
    }
}

check_log_stopped();

exit !$good;


sub init_d
{
    my ($io) = @_;

    while (my $line = <$io>) {
        if ($ENV{SK_TESTS_VERBOSE}) {
            print STDERR $line;
        }
        chomp $line;

        if ($expected_status eq 'running') {
            if ($line =~ /$DAEMON is running with pid (\d+)/) {
                if ($pid) {
                    if ($pid ne $1) {
                        print STDERR ("$NAME: Process ID mismatch:",
                                      " file '$pid' vs script '$1'\n");
                        $good = 0;
                    }
                }
                else {
                    $pid = $1;
                }
            }
            else {
                print STDERR
                    "$NAME: Unexpected $expected_status line '$line'\n";
                $good = 0;
            }
        }
        elsif ($expected_status eq 'starting') {
            unless ($line =~ /Starting $DAEMON:\t\[OK\]/) {
                print STDERR
                    "$NAME: Unexpected $expected_status line '$line'\n";
                $good = 0;
            }
        }
        elsif ($expected_status eq 'stopped') {
            unless ($line =~ /$DAEMON is stopped/) {
                print STDERR
                    "$NAME: Unexpected $expected_status line '$line'\n";
                $good = 0;
            }
        }
        elsif ($expected_status eq 'stopping') {
            unless ($line =~ /Stopping $DAEMON:\t\[OK\]/) {
                print STDERR
                    "$NAME: Unexpected $expected_status line '$line'\n";
                $good = 0;
            }
        }
        else {
            die "$NAME: Unknown \$expected_status '$expected_status'\n";
        }
    }

    close $io;
    if ($expected_status eq 'running') {
        unless ($? == 0) {
            print STDERR "$NAME: Unexpected $expected_status status $?\n";
            $good = 0;
        }
    }
    elsif ($expected_status eq 'starting') {
        unless ($? == 0) {
            print STDERR "$NAME: Unexpected $expected_status status $?\n";
            $good = 0;
        }
    }
    elsif ($expected_status eq 'stopped') {
        unless ($? == (1 << 8)) {
            print STDERR "$NAME: Unexpected $expected_status status $?\n";
            $good = 0;
        }
    }
    elsif ($expected_status eq 'stopping') {
        unless ($? == (1 << 8)) {
            print STDERR "$NAME: Unexpected $expected_status status $?\n";
            $good = 0;
        }
    }

    if ($ENV{SK_TESTS_VERBOSE}) {
        print STDERR "status = $?\n";
    }
}


sub kill_process
{
    my ($pid) = @_;

    if (!kill 0, $pid) {
        return;
    }
    if ($ENV{SK_TESTS_VERBOSE}) {
        print STDERR "Sending SIGTERM to process $pid\n";
    }
    kill 15, $pid;
    sleep 2;
    if (kill 0, $pid) {
        if ($ENV{SK_TESTS_VERBOSE}) {
            print STDERR "Sending SIGKILL to process $pid\n";
        }
        kill 9, $pid;
    }
}


sub check_log_stopped
{
    my @log_files = sort glob("$log_dir/$DAEMON-*.log");
    my $log = pop @log_files;
    open LOG, $log
        or die "$NAME: Cannot open log file '$log': $!\n";
    my $last_line;
    while (defined(my $line = <LOG>)) {
        $last_line = $line;
    }
    close LOG;
    unless (defined $last_line) {
        die "$NAME: Log file '$log' is empty\n";
    }
    if ($last_line !~ /Stopped logging/) {
        die "$NAME: Log file '$log' does not end correctly\n";
    }
}

