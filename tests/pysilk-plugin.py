# RCSIDENT("$SiLK: pysilk-plugin.py a079c82ff568 2011-06-24 15:22:59Z mthomas $")

import sys,struct,os,silk

# FILTERING
#
# passes records that have the same sport and dport. when finished,
# prints to stderr the number of unique same-port combinations it saw

saw_port = {}
unique_ports = 0

def filter_same_port(r):
    global saw_port, unique_ports
    if r.sport != r.dport:
        return False
    if r.sport not in saw_port:
        unique_ports += 1
        saw_port[r.sport] = 1
    return True

def finalize_same_port():
    global unique_ports
    sys.stderr.write("Saw %d same-port combinations\n" % unique_ports)

register_filter(filter_same_port, finalize=finalize_same_port)


# FIELD (KEY)
#
# 'lower_port' returns the lower port for each flow.  presumably this
# would be the service port.

def lower_port_rec_to_text(r):
    if r.sport < r.dport:
        return "%d" % r.sport
    return "%d" % r.dport

def lower_port_rec_to_bin(r):
    if r.sport < r.dport:
        return struct.pack("!H", r.sport)
    return struct.pack("!H", r.dport)

def lower_port_bin_to_text(b):
    return "%d" % struct.unpack("!H", b)

register_field('lower_port', column_width=5, bin_bytes = struct.calcsize("!H"),
               bin_to_text = lower_port_bin_to_text,
               rec_to_bin = lower_port_rec_to_bin,
               rec_to_text = lower_port_rec_to_text)


# AGGREGATE VALUE
#
# 'max_bytes' is the maximum byte count we see across all flows in the
# bin

def max_bytes_check(r, packed_max):
    (max_bytes,) = struct.unpack("I", packed_max)
    if r.bytes > max_bytes:
        packed_max = struct.pack("I", r.bytes)
    return packed_max

def max_bytes_merge(packed_max1, packed_max2):
    (max_bytes1,) = struct.unpack("I", packed_max1)
    (max_bytes2,) = struct.unpack("I", packed_max2)
    if max_bytes1 > max_bytes2:
        return max_bytes1
    return max_bytes2

def max_bytes_compare(packed_max1, packed_max2):
    (max_bytes1,) = struct.unpack("I", packed_max1)
    (max_bytes2,) = struct.unpack("I", packed_max2)
    if max_bytes1 > max_bytes2:
        return 1
    if max_bytes1 < max_bytes2:
        return -1
    return 0

def max_bytes_print(packed_max):
    (max_bytes,) = struct.unpack("I", packed_max)
    return "%d" % max_bytes

register_field("max_bytes", column_width = 10, bin_bytes = struct.calcsize("I"),
               add_rec_to_bin = max_bytes_check, bin_to_text = max_bytes_print,
               bin_merge = max_bytes_merge, bin_compare = max_bytes_compare,
               initial_value = struct.pack("I", 0))


# SIMPLIFIED FIELD REGISTRATION

# 'server_ip' and 'server_ipv6' are the sip if sport < dport,
# otherwise dip.

def lowport_ip_addr(r):
    if r.sport < r.dport:
        return r.sip
    else:
        return r.dip
register_ipv4_field("server_ip", lowport_ip_addr)

if silk.ipv6_enabled():
    register_ip_field("server_ipv6", lowport_ip_addr)

# 'lower_port_simple' returns the lower port for each flow.
# presumably this would be the service port.

def lower_port_simple(r):
    return min(r.sport, r.dport)

register_int_field("lower_port_simple", lower_port_simple, 0, 0xffff)

# 'proto_name' returns the name of the protocol if known, or the
# number otherwise.

protocols = {6: 'TCP', 17: 'UDP',  1: 'ICMP'}
sorted_proto = list(protocols.values())
sorted_proto.sort()

def proto_name(r):
    return protocols.get(r.protocol, str(r.protocol))

register_enum_field("proto_name", proto_name, 4, sorted_proto)

# 'large_packet_flows' sums up the number of flows whose average
# packet size is greater than 1000 bytes.
#

def large_packet_aggregator(r):
    if r.bytes // r.packets > 1000:
        return 1
    return 0

register_int_sum_aggregator("large_packet_flows", large_packet_aggregator)

# 'largest_packets' is the largest average packet size
# 'smallest_packets' is the smallest average packet size

def avg_packet(r):
    return int(r.bytes // r.packets)

register_int_max_aggregator("largest_packets", avg_packet)
register_int_min_aggregator("smallest_packets", avg_packet)


# Country Code

if os.getenv("SILK_COUNTRY_CODES"):
    try:
        silk.init_country_codes()
        register_enum_field("py-scc", lambda rec: rec.sip.country_code(), 6)
        register_enum_field("py-dcc", lambda rec: rec.dip.country_code(), 6)
    except:
        if os.getenv("SILK_PYTHON_TRACEBACK"):
            raise
