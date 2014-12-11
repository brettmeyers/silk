%{
/*
** Copyright (C) 2005-2014 by Carnegie Mellon University.
**
** @OPENSOURCE_HEADER_START@
**
** Use of the SILK system and related source code is subject to the terms
** of the following licenses:
**
** GNU Public License (GPL) Rights pursuant to Version 2, June 1991
** Government Purpose License Rights (GPLR) pursuant to DFARS 252.227.7013
**
** NO WARRANTY
**
** ANY INFORMATION, MATERIALS, SERVICES, INTELLECTUAL PROPERTY OR OTHER
** PROPERTY OR RIGHTS GRANTED OR PROVIDED BY CARNEGIE MELLON UNIVERSITY
** PURSUANT TO THIS LICENSE (HEREINAFTER THE "DELIVERABLES") ARE ON AN
** "AS-IS" BASIS. CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY
** KIND, EITHER EXPRESS OR IMPLIED AS TO ANY MATTER INCLUDING, BUT NOT
** LIMITED TO, WARRANTY OF FITNESS FOR A PARTICULAR PURPOSE,
** MERCHANTABILITY, INFORMATIONAL CONTENT, NONINFRINGEMENT, OR ERROR-FREE
** OPERATION. CARNEGIE MELLON UNIVERSITY SHALL NOT BE LIABLE FOR INDIRECT,
** SPECIAL OR CONSEQUENTIAL DAMAGES, SUCH AS LOSS OF PROFITS OR INABILITY
** TO USE SAID INTELLECTUAL PROPERTY, UNDER THIS LICENSE, REGARDLESS OF
** WHETHER SUCH PARTY WAS AWARE OF THE POSSIBILITY OF SUCH DAMAGES.
** LICENSEE AGREES THAT IT WILL NOT MAKE ANY WARRANTY ON BEHALF OF
** CARNEGIE MELLON UNIVERSITY, EXPRESS OR IMPLIED, TO ANY PERSON
** CONCERNING THE APPLICATION OF OR THE RESULTS TO BE OBTAINED WITH THE
** DELIVERABLES UNDER THIS LICENSE.
**
** Licensee hereby agrees to defend, indemnify, and hold harmless Carnegie
** Mellon University, its trustees, officers, employees, and agents from
** all claims or demands made against them (and any related losses,
** expenses, or attorney's fees) arising out of, or relating to Licensee's
** and/or its sub licensees' negligent use or willful misuse of or
** negligent conduct or willful misconduct regarding the Software,
** facilities, or other rights or assistance granted by Carnegie Mellon
** University under this License, including, but not limited to, any
** claims of product liability, personal injury, death, damage to
** property, or violation of any laws or regulations.
**
** Carnegie Mellon University Software Engineering Institute authored
** documents are sponsored by the U.S. Department of Defense under
** Contract FA8721-05-C-0003. Carnegie Mellon University retains
** copyrights in all material produced under this contract. The U.S.
** Government retains a non-exclusive, royalty-free license to publish or
** reproduce these documents, or allow others to do so, for U.S.
** Government purposes only pursuant to the copyright license under the
** contract clause at 252.227.7013.
**
** @OPENSOURCE_HEADER_END@
*/

/*
**  Parser for probe configuration file
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: probeconfparse.y cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/libflowsource.h>
#include <silk/probeconf.h>
#include <silk/skipaddr.h>
#include <silk/sklog.h>
#include <silk/sksite.h>
#include "probeconfscan.h"


/* LOCAL DEFINES AND TYPEDEFS */

/* Set DEBUG to 1 to enable debugging printf messasges, 0 otherwise.
 * Generally best to leave this commented out so gcc -DDEBUG=1 will
 * work */
/* #define DEBUG 1 */

/* Verify DEBUG is set */
#ifndef DEBUG
#  define DEBUG 0
#endif

/* For printing messages when DEBUG is non-zero.  Use as:
 *    DEBUG_PRINTF(("x is %d\n", x));
 * Note ((double parens)) */
#if DEBUG
#  define DEBUG_PRINTF(x) printf x
#else
#  define DEBUG_PRINTF(x)
#endif


/* magic value used to denote that a uint16_t---which are stored in
 * uint32_t's in the parser---has not yet been given a value. */
#define UINT16_NO_VALUE  0x10000  /* 0xFFFF + 1 */

/*
 * sk_vector_t of IDs are created as they are needed, but to avoid
 * repeatedly creating and destroying the vectors, "deleted" vectors are
 * added to a pool.  When creating a vector, the code will check the
 * pool before allocating.  This macro is the size of the pool; if a
 * vector is "deleted" and the pool is full, the vector is free()ed.
 */
#define VECTOR_POOL_CAPACITY  16


typedef struct vector_pool_st {
    sk_vector_t    *pool[VECTOR_POOL_CAPACITY];
    size_t          element_size;
    int             count;
} vector_pool_t;


/* LOCAL VARIABLES */

/* number of errors in current defn */
static int defn_errors = 0;

/* the vector pools */
static vector_pool_t ptr_vector_pool;
static vector_pool_t *ptr_pool = &ptr_vector_pool;
static vector_pool_t u32_vector_pool;
static vector_pool_t *u32_pool = &u32_vector_pool;

/* The parser works on a single global probe, sensor, or group */
static skpc_probe_t *probe = NULL;
static skpc_sensor_t *sensor = NULL;
static skpc_group_t *group = NULL;

/* Place to stash listen_as address and port until end of probe is
 * reached */
static char *listen_as_address = NULL;
static char *listen_port = NULL;

/* LOCAL FUNCTION PROTOTYPES */

static sk_vector_t *vectorPoolGet(vector_pool_t *pool);
static void vectorPoolPut(vector_pool_t *pool, sk_vector_t *v);
static void vectorPoolEmpty(vector_pool_t *pool);


static void missing_value(void);


/* include a file */
static void include_file(char *name);

/* functions to set attributes of a probe_attr_t */
static void
probe_begin(
    char               *probe_name,
    char               *probe_type);
static void probe_end(void);
static void probe_priority(sk_vector_t *v);
static void probe_protocol(sk_vector_t *v);
static void probe_listen_as_host(sk_vector_t *v);
static void probe_listen_on_port(sk_vector_t *v);
static void probe_listen_on_usocket(sk_vector_t *v);
static void probe_read_from_file(sk_vector_t *v);
static void probe_poll_directory(sk_vector_t *v);
static void probe_accept_from_host(sk_vector_t *v);
static void probe_log_flags(uint32_t n);
static void probe_interface_values(sk_vector_t *v);
static void probe_quirks(sk_vector_t *v);

static void
sensor_begin(
    char               *sensor_name);
static void sensor_end(void);
static void sensor_isp_ip(sk_vector_t *v);
static void sensor_interface(char *name, sk_vector_t *list);
static void
sensor_ipblock(
    char               *name,
    sk_vector_t        *wl,
    int                 negated);
static void sensor_filter(skpc_filter_t filter, sk_vector_t *v);
static void sensor_network(skpc_direction_t direction, char *name);
static void sensor_probes(char *probe_type, sk_vector_t *v);

static void
group_begin(
    char               *group_name);
static void group_end(void);
static void
group_add_data(
    sk_vector_t        *v,
    skpc_group_type_t   g_type);

static skpc_group_t *
get_group(
    const char         *g_name,
    skpc_group_type_t   g_type);
static int
add_values_to_group(
    skpc_group_t       *g,
    sk_vector_t        *v,
    skpc_group_type_t   g_type);


/* functions to convert string input to another form */
static uint32_t parse_int_u16(char *s);
static int vectorSingleString(sk_vector_t *v, char **s);
static int parse_ip_addr(char *s, uint32_t *ip);
static skIPWildcard_t *parse_wildcard_addr(char *s);
static uint32_t parse_log_flag(char *s);

/* add a new flag to the log flags */
static uint32_t log_flags_add_flag(uint32_t old_val, uint32_t value);


%}
%union {
    char               *string;
    sk_vector_t        *vector;
    uint32_t            u32;
    skpc_direction_t    net_dir;
    skpc_filter_t       filter;
}

%token ACCEPT_FROM_HOST_T
%token COMMA
%token END_GROUP_T
%token END_PROBE_T
%token END_SENSOR_T
%token EOL
%token GROUP_T
%token INCLUDE_T
%token INTERFACES_T
%token INTERFACE_VALUES_T
%token IPBLOCKS_T
%token ISP_IP_T
%token LISTEN_AS_HOST_T
%token LISTEN_ON_PORT_T
%token LISTEN_ON_USOCKET_T
%token LOG_FLAGS_T
%token POLL_DIRECTORY_T
%token PRIORITY_T
%token PROBE_T
%token PROTOCOL_T
%token QUIRKS_T
%token READ_FROM_FILE_T
%token REMAINDER_T
%token SENSOR_T
%token <string> ID
%token <string> NET_NAME_INTERFACE
%token <string> NET_NAME_IPBLOCK
%token <string> PROBES
%token <string> QUOTED_STRING
%token <net_dir> NET_DIRECTION
%token <filter> FILTER;

%token ERR_STR_TOO_LONG

%type <u32>         log_flags
%type <vector>      id_list


%%

    /*
     * ******************  GRAMMAR RULES  ***********************************
     */

input:                                    probe_defn
                                        | sensor_defn
                                        | group_defn
                                        | include_stmt
                                        | input probe_defn
                                        | input sensor_defn
                                        | input group_defn
                                        | input include_stmt
                                        | error
{
    skpcParseErr("Misplaced or unrecognized keyword");
    ++pcscan_errors;
};


    /*
     * Include <FILE>
     */

include_stmt:                             INCLUDE_T QUOTED_STRING EOL
{
    include_file($2);
}
                                        | INCLUDE_T EOL
{
    missing_value();
};


    /*
     * A probe
     */

probe_defn:                               probe_begin probe_stmts probe_end
;

probe_begin:                              PROBE_T ID ID EOL
{
    probe_begin($2, $3);
}
                                        | PROBE_T ID EOL
{
    /* error */
    probe_begin(NULL, $2);
}
                                        | PROBE_T EOL
{
    /* error */
    probe_begin(NULL, NULL);
};

probe_end:                                END_PROBE_T EOL
{
    probe_end();
}
                                        | END_GROUP_T EOL
{
    ++defn_errors;
    skpcParseErr("%s used to close probe", pcscan_clause);
    probe_end();
};
                                        | END_SENSOR_T EOL
{
    ++defn_errors;
    skpcParseErr("%s used to close probe", pcscan_clause);
    probe_end();
};


probe_stmts:                              /* empty */
                                        | probe_stmts probe_stmt
;

probe_stmt:                               stmt_probe_priority
                                        | stmt_probe_protocol
                                        | stmt_probe_listen_host
                                        | stmt_probe_listen_port
                                        | stmt_probe_listen_usocket
                                        | stmt_probe_read_file
                                        | stmt_probe_poll_directory
                                        | stmt_probe_accept_host
                                        | stmt_probe_log_flags
                                        | stmt_probe_interface_values
                                        | stmt_probe_quirks
                                        | error
{
    ++defn_errors;
    skpcParseErr(("Error in probe %s:"
                  " Missing \"end probe\" or invalid keyword or value"),
                 (probe ? skpcProbeGetName(probe) : "block"));
};


stmt_probe_priority:                      PRIORITY_T id_list EOL
{
    probe_priority($2);
}
                                        | PRIORITY_T EOL
{
    missing_value();
};

stmt_probe_protocol:                      PROTOCOL_T id_list EOL
{
    probe_protocol($2);
}
                                        | PROTOCOL_T EOL
{
    missing_value();
};

stmt_probe_listen_host:                   LISTEN_AS_HOST_T id_list EOL
{
    probe_listen_as_host($2);
}
                                        | LISTEN_AS_HOST_T EOL
{
    missing_value();
};

stmt_probe_listen_port:                   LISTEN_ON_PORT_T id_list EOL
{
    probe_listen_on_port($2);
}
                                        | LISTEN_ON_PORT_T EOL
{
    missing_value();
};

stmt_probe_listen_usocket:                LISTEN_ON_USOCKET_T id_list EOL
{
    probe_listen_on_usocket($2);
}
                                        | LISTEN_ON_USOCKET_T EOL
{
    missing_value();
};

stmt_probe_read_file:                     READ_FROM_FILE_T id_list EOL
{
    probe_read_from_file($2);
}
                                        | READ_FROM_FILE_T EOL
{
    missing_value();
};

stmt_probe_poll_directory:                POLL_DIRECTORY_T id_list EOL
{
    probe_poll_directory($2);
}
                                        | POLL_DIRECTORY_T EOL
{
    missing_value();
};

stmt_probe_accept_host:                   ACCEPT_FROM_HOST_T id_list EOL
{
    probe_accept_from_host($2);
}
                                        | ACCEPT_FROM_HOST_T EOL
{
    missing_value();
};


stmt_probe_log_flags:                     LOG_FLAGS_T log_flags EOL
{
    probe_log_flags($2);
}
                                        | LOG_FLAGS_T EOL
{
    missing_value();
};

stmt_probe_interface_values:              INTERFACE_VALUES_T id_list EOL
{
    probe_interface_values($2);
}
                                        | INTERFACE_VALUES_T EOL
{
    missing_value();
};

stmt_probe_quirks:                        QUIRKS_T id_list EOL
{
    probe_quirks($2);
}
                                        | QUIRKS_T EOL
{
    missing_value();
};


    /*
     * A sensor
     */

sensor_defn:                              sensor_begin sensor_stmts sensor_end
;

sensor_stmts:                           /* empty */
                                        | sensor_stmts sensor_stmt
;

sensor_stmt:                              stmt_sensor_isp_ip
                                        | stmt_sensor_interface
                                        | stmt_sensor_ipblock
                                        | stmt_sensor_filter
                                        | stmt_sensor_network
                                        | stmt_sensor_probes
                                        | error
{
    ++defn_errors;
    skpcParseErr(("Error in sensor %s:"
                  " Missing \"end sensor\" or invalid keyword or value"),
                 (sensor ? skpcSensorGetName(sensor) : "block"));
};


sensor_begin:                             SENSOR_T ID EOL
{
    sensor_begin($2);
}
                                        | SENSOR_T EOL
{
    sensor_begin(NULL);
};

sensor_end:                               END_SENSOR_T EOL
{
    sensor_end();
}
                                        | END_GROUP_T EOL
{
    ++defn_errors;
    skpcParseErr("%s used to close sensor", pcscan_clause);
    sensor_end();
}
                                        | END_PROBE_T EOL
{
    ++defn_errors;
    skpcParseErr("%s used to close sensor", pcscan_clause);
    sensor_end();
};

stmt_sensor_isp_ip:                       ISP_IP_T id_list EOL
{
    sensor_isp_ip($2);
}
                                        | ISP_IP_T EOL
{
    missing_value();
};

stmt_sensor_interface:                    NET_NAME_INTERFACE id_list EOL
{
    sensor_interface($1, $2);
}
                                        | NET_NAME_INTERFACE REMAINDER_T EOL
{
    sensor_interface($1, NULL);
}
                                        | NET_NAME_INTERFACE EOL
{
    missing_value();
    if ($1) {
        free($1);
    }
};

stmt_sensor_ipblock:                      NET_NAME_IPBLOCK id_list EOL
{
    sensor_ipblock($1, $2, 0);
}
                                        | NET_NAME_IPBLOCK REMAINDER_T EOL
{
    sensor_ipblock($1, NULL, 0);
}
                                        | NET_NAME_IPBLOCK EOL
{
    missing_value();
    if ($1) {
        free($1);
    }
};

stmt_sensor_filter:                       FILTER id_list EOL
{
    /* discard-{when,unless} {source,destination,any}-{interfaces,ipblocks} */
    sensor_filter($1, $2);
}
                                        | FILTER EOL
{
    missing_value();
};

stmt_sensor_network:                      NET_DIRECTION ID EOL
{
    sensor_network($1, $2);
}
                                        | NET_DIRECTION EOL
{
    missing_value();
};

stmt_sensor_probes:                       PROBES id_list EOL
{
    sensor_probes($1, $2);
}
                                        | PROBES EOL
{
    missing_value();
    if ($1) {
        free($1);
    }
};


    /*
     *  A group
     */

group_defn:                               group_begin group_stmts group_end
;

group_stmts:                            /* empty */
                                        | group_stmts group_stmt
;

group_stmt:                               stmt_group_interfaces
                                        | stmt_group_ipblocks
                                        | error
{
    ++defn_errors;
    skpcParseErr(("Error in group %s:"
                  " Missing \"end group\" or invalid keyword or value"),
                 (group ? skpcGroupGetName(group) : "block"));
};


group_begin:                              GROUP_T ID EOL
{
    group_begin($2);
}
                                        | GROUP_T EOL
{
    group_begin(NULL);
};

group_end:                                END_GROUP_T EOL
{
    group_end();
}
                                        | END_PROBE_T EOL
{
    ++defn_errors;
    skpcParseErr("%s used to close group", pcscan_clause);
    group_end();
};
                                        | END_SENSOR_T EOL
{
    ++defn_errors;
    skpcParseErr("%s used to close group", pcscan_clause);
    group_end();
};

stmt_group_interfaces:                    INTERFACES_T id_list EOL
{
    group_add_data($2, SKPC_GROUP_INTERFACE);
}
                                        | INTERFACES_T EOL
{
    missing_value();
};


stmt_group_ipblocks:                      IPBLOCKS_T id_list EOL
{
    group_add_data($2, SKPC_GROUP_IPBLOCK);
}
                                        | IPBLOCKS_T EOL
{
    missing_value();
};



    /*
     *  Parse a list of strings, which are separated by whitespace
     *  and/or commas.  Return as a sk_vector_t*
     */

id_list:                                  ID
{
    sk_vector_t *v = vectorPoolGet(ptr_pool);
    char *s = $1;
    skVectorAppendValue(v, &s);
    $$ = v;
}
                                        | id_list ID
{
    char *s = $2;
    skVectorAppendValue($1, &s);
    $$ = $1;
}
                                        | id_list COMMA ID
{
    char *s = $3;
    skVectorAppendValue($1, &s);
    $$ = $1;
};


    /*
     *  Parse log flags: either a single flag or a list of flags
     */

log_flags:                                ID
{
    $$ = parse_log_flag($1);
}
                                        | log_flags ID
{
    uint32_t n = parse_log_flag($2);
    n = log_flags_add_flag($1, n);
    $$ = n;
}
                                        | log_flags COMMA ID
{
    uint32_t n = parse_log_flag($3);
    n = log_flags_add_flag($1, n);
    $$ = n;
};


%%

/*
 * *******************   SUPPORTING CODE  ******************************
 */


/*
 *  *****  Pool of sk_vector_t  ****************************************
 */


/* get a new vector from the free-pool; if pool is empty, create
 * a new vector */
static sk_vector_t *
vectorPoolGet(
    vector_pool_t      *pool)
{
    sk_vector_t *v;

    /* if there are any in the pool, return one.  Otherwise create one. */
    if (pool->count) {
        --pool->count;
        v = pool->pool[pool->count];
        skVectorClear(v);
    } else {
        v = skVectorNew(pool->element_size);
    }

    return v;
}


/* add the vector to the free-pool.  If the pool is full, just
 * destroy the vector. */
static void
vectorPoolPut(
    vector_pool_t      *pool,
    sk_vector_t        *v)
{
    assert(pool);
    assert(v);
    assert(pool->element_size == skVectorGetElementSize(v));

    /* If the pool is full, destroy the list we were handed; otherwise
     * add it to the pool. */
    if (pool->count == VECTOR_POOL_CAPACITY) {
        skVectorDestroy(v);
    } else {
        pool->pool[pool->count] = v;
        ++pool->count;
    }
}


/* remove all vector's from the pool and destroy them */
static void
vectorPoolEmpty(
    vector_pool_t      *pool)
{
    int i;

    for (i = 0; i < pool->count; ++i) {
        skVectorDestroy(pool->pool[i]);
    }
    pool->count = 0;
}


static void
missing_value(
    void)
{
    ++defn_errors;
    skpcParseErr("Missing arguments for %s statement", pcscan_clause);
}


static void
include_file(
    char               *filename)
{
    skpcParseIncludePush(filename);
}



/*
 *  *****  Probes  *****************************************************
 */


static void
set_listen_data(
    void)
{
    char buf[1024];
    int rv;
    sk_sockaddr_array_t *sa;

    if (listen_port == NULL) {
        if (listen_as_address == NULL) {
            sa = NULL;
        } else {
            rv = skStringParseHostPortPair(&sa, listen_as_address,
                                           HOST_REQUIRED | PORT_PROHIBITED);
            if (rv != 0) {
                skpcParseErr("Invalid listen-as-host '%s': %s",
                             listen_as_address, skStringParseStrerror(rv));
                ++defn_errors;
                return;
            }
        }
    } else {
        if (listen_as_address == NULL) {
            rv = skStringParseHostPortPair(&sa, listen_port,
                                           PORT_REQUIRED | HOST_PROHIBITED);
            if (rv != 0) {
                skpcParseErr("Invalid listen-on-port '%s': %s",
                             listen_port, skStringParseStrerror(rv));
                ++defn_errors;
                return;
            }
        } else {
            rv = snprintf(buf, sizeof(buf), "[%s]:%s",
                          listen_as_address, listen_port);
            if ((size_t)rv >= sizeof(buf)) {
                skpcParseErr("Length of listen-as-host or listen-on-port "
                             "is too large");
                ++defn_errors;
                return;
            }
            rv = skStringParseHostPortPair(&sa, buf, PORT_REQUIRED);
            if (rv != 0) {
                skpcParseErr(("Invalid listen-as-host or listen-on-port"
                              " '%s': %s"),
                             buf, skStringParseStrerror(rv));
                ++defn_errors;
                return;
            }
        }
    }
    rv = skpcProbeSetListenOnSockaddr(probe, sa);
    if (rv != 0) {
        skpcParseErr("Error setting listen address or port");
        ++defn_errors;
    }
}


/* complete the current probe */
static void
probe_end(
    void)
{
    if (!probe) {
        skpcParseErr("No active probe in %s statement", pcscan_clause);
        goto END;
    }

    if (defn_errors) {
        goto END;
    }

    if (skpcProbeVerify(probe, 0)) {
        skpcParseErr("Unable to verify probe '%s'",
                     skpcProbeGetName(probe));
        ++defn_errors;
        goto END;
    }

    /* Probe is valid and now owned by probeconf.  Set to NULL so it
     * doesn't get free()'ed. */
    probe = NULL;

  END:
    if (defn_errors) {
        skAppPrintErr("Encountered %d error%s while processing probe '%s'",
                      defn_errors, ((defn_errors == 1) ? "" : "s"),
                      (probe ? skpcProbeGetName(probe) : ""));
        pcscan_errors += defn_errors;
        defn_errors = 0;
    }
    if (probe) {
        skpcProbeDestroy(&probe);
        probe = NULL;
    }
    if (listen_as_address) {
        free(listen_as_address);
        listen_as_address = NULL;
    }
    if (listen_port) {
        free(listen_port);
        listen_port = NULL;
    }
}


/* Begin a new probe by setting the memory of the global probe_attr_t
 * into its initial state. */
static void
probe_begin(
    char               *probe_name,
    char               *probe_type)
{
    const char *dummy_name = "<ERROR>";
    skpc_probetype_t t;

    if (probe) {
        skpcParseErr("Found active probe in %s statement", pcscan_clause);
        skpcProbeDestroy(&probe);
        probe = NULL;
    }
    defn_errors = 0;

    if (skpcProbeCreate(&probe)) {
        skpcParseErr("Fatal: Unable to create probe");
        exit(EXIT_FAILURE);
    }

    /* probe_name will only be NULL on bad input from user */
    if (probe_name == NULL) {
        skpcParseErr("%s requires a name and a type", pcscan_clause);
        ++defn_errors;
        if (probe_type == NULL) {
            skpcProbeSetName(probe, dummy_name);
        } else if (skpcProbeSetName(probe, probe_type)) {
            skpcParseErr("Error setting probe name to %s", probe_type);
            ++defn_errors;
        }
        goto END;
    }

    if (skpcProbeLookupByName(probe_name)) {
        skpcParseErr("A probe named '%s' already exists", probe_name);
        ++defn_errors;
    }

    if (skpcProbeSetName(probe, probe_name)) {
        skpcParseErr("Error setting probe name to %s", probe_name);
        ++defn_errors;
    }
    free(probe_name);

    t = skpcProbetypeNameToEnum(probe_type);
    if (t == PROBE_ENUM_INVALID) {
        skpcParseErr("Do not recognize probe type '%s'", probe_type);
        ++defn_errors;
    } else if (skpcProbeSetType(probe, t)) {
        skpcParseErr("Error setting probe type to %s", probe_type);
        ++defn_errors;
    }

    if (listen_as_address != NULL) {
        free(listen_as_address);
        listen_as_address = NULL;
    }
    if (listen_port) {
        free(listen_port);
        listen_port = NULL;
    }

  END:
    free(probe_type);
}


/*
 *  probe_priority(s);
 *
 *    Set the priority of the global probe to s.
 */
static void
probe_priority(
    sk_vector_t        *v)
{
    uint32_t n;
    char *s;

    if (vectorSingleString(v, &s)) {
        return;
    }

    n = parse_int_u16(s);
    if (n == UINT16_NO_VALUE) {
        ++defn_errors;
        return;
    }

    /* Priority is no longer used; just ignore it */
}


/*
 *  probe_protocol(s);
 *
 *    Set the probe-type of the global probe to 's'.
 */
static void
probe_protocol(
    sk_vector_t        *v)
{
    skpc_proto_t proto;
    char *s;

    if (vectorSingleString(v, &s)) {
        return;
    }

    proto = skpcProtocolNameToEnum(s);
    if (proto == SKPC_PROTO_UNSET) {
        skpcParseErr("Do not recognize protocol '%s'", s);
        ++defn_errors;
    } else if (skpcProbeSetProtocol(probe, proto)) {
        skpcParseErr("Error setting %s value for probe '%s' to '%s'",
                     pcscan_clause, skpcProbeGetName(probe), s);
        ++defn_errors;
    }

    free(s);
}


/*
 *  probe_listen_as_host(s);
 *
 *    Set the global probe to listen for flows as the host IP 's'.
 */
static void
probe_listen_as_host(
    sk_vector_t        *v)
{
    char *s;

    if (vectorSingleString(v, &s)) {
        return;
    }
    if (listen_as_address != NULL) {
        free(s);
    }
    listen_as_address = s;

    set_listen_data();
}


/*
 *  probe_listen_on_port(s);
 *
 *    Set the global probe to listen for flows on port 's'.
 */
static void
probe_listen_on_port(
    sk_vector_t        *v)
{
    char *s;

    if (vectorSingleString(v, &s)) {
        return;
    }
    if (listen_port != NULL) {
        free(listen_port);
    }
    listen_port = s;

    set_listen_data();
}


/*
 *  probe_listen_on_usocket(s);
 *
 *    Set the global probe to listen for flows on the unix domain socket 's'.
 */
static void
probe_listen_on_usocket(
    sk_vector_t        *v)
{
    char *s;

    if (vectorSingleString(v, &s)) {
        return;
    }
    if (skpcProbeSetListenOnUnixDomainSocket(probe, s)) {
        skpcParseErr("Error setting %s value for probe '%s'",
                     pcscan_clause, skpcProbeGetName(probe));
        ++defn_errors;
    }
    free(s);
}


/*
 *  probe_read_from_file(s);
 *
 *    Set the global probe to read flows from the file 's'.
 */
static void
probe_read_from_file(
    sk_vector_t        *v)
{
    char *s;

    if (vectorSingleString(v, &s)) {
        return;
    }
    if (skpcProbeSetFileSource(probe, s)) {
        skpcParseErr("Error setting %s value for probe '%s'",
                     pcscan_clause, skpcProbeGetName(probe));
        ++defn_errors;
    }
    free(s);
}


/*
 *  probe_poll_directory(s);
 *
 *    Set the global probe to read flows from files that appear in the
 *    directory 's'.
 */
static void
probe_poll_directory(
    sk_vector_t        *v)
{
    char *s;

    if (vectorSingleString(v, &s)) {
        return;
    }
    if (skpcProbeSetPollDirectory(probe, s)) {
        skpcParseErr("Error setting %s value for probe '%s'",
                     pcscan_clause, skpcProbeGetName(probe));
        ++defn_errors;
    }
    free(s);
}


/*
 *  probe_accept_from_host(s);
 *
 *    Set the global probe to accept data from the host s.
 */
static void
probe_accept_from_host(
    sk_vector_t        *v)
{
    sk_sockaddr_array_t *sa;
    int rv;
    char *s;

    if (vectorSingleString(v, &s)) {
        return;
    }

    rv = skStringParseHostPortPair(&sa, s, HOST_REQUIRED | PORT_PROHIBITED);
    if (rv != 0) {
        skpcParseErr("Unable to resolve %s value '%s': %s",
                     pcscan_clause, s, skStringParseStrerror(rv));
        ++defn_errors;
        free(s);
        return;
    }

    rv = skpcProbeSetAcceptFromHost(probe, sa);
    if (rv != 0) {
        skpcParseErr("Error setting %s value for probe '%s'",
                     pcscan_clause, skpcProbeGetName(probe));
        ++defn_errors;
    }
    free(s);
}


/*
 *  probe_log_flags(n);
 *
 *    Set the log flags on the probe to 'n';
 */
static void
probe_log_flags(
    uint32_t            n)
{
    if (n == UINT16_NO_VALUE) {
        /* some error in parsing log flags */
        ++defn_errors;
        return;
    }

    if (skpcProbeSetLogFlags(probe, n)) {
        skpcParseErr("Error setting %s value for probe '%s'",
                     pcscan_clause, skpcProbeGetName(probe));
        ++defn_errors;
    }
}


/*
 *  probe_interface_values(s);
 *
 *    Set the global probe to store either snmp or vlan values in the
 *    'input' and 'output' interface fields on SiLK Flow records.
 */
static void
probe_interface_values(
    sk_vector_t        *v)
{
    skpc_ifvaluetype_t ifvalue = SKPC_IFVALUE_SNMP;
    char *s;

    if (vectorSingleString(v, &s)) {
        return;
    }

    if (0 == strcmp(s, "snmp")) {
        ifvalue = SKPC_IFVALUE_SNMP;
    } else if (0 == strcmp(s, "vlan")) {
        ifvalue = SKPC_IFVALUE_VLAN;
    } else {
        skpcParseErr("Invalid %s value '%s'",
                     pcscan_clause, s);
        ++defn_errors;
        goto END;
    }

    if (skpcProbeSetInterfaceValueType(probe, ifvalue)) {
        skpcParseErr("Unable to set %s value '%s'",
                     pcscan_clause, s);
        ++defn_errors;
        goto END;
    }

  END:
    free(s);
}


/*
 *  probe_quirks(s);
 *
 *    Set the "quirks" field on the global probe.
 */
static void
probe_quirks(
    sk_vector_t        *v)
{
    size_t count = skVectorGetCount(v);
    size_t i;
    char **s;
    int rv;

    /* clear any existing quirks */
    skpcProbeClearQuirks(probe);

    /* loop over the list of quirks and add to the probe */
    for (i = 0; i < count; ++i) {
        s = (char**)skVectorGetValuePointer(v, i);
        rv = skpcProbeAddQuirk(probe, *s);
        switch (rv) {
          case 0:
            break;
          case -1:
            skpcParseErr("Invalid %s value '%s'",
                         pcscan_clause, *s);
            ++defn_errors;
            break;
          case -2:
            skpcParseErr("Invalid %s combination",
                         pcscan_clause);
            ++defn_errors;
            break;
          default:
            skAbortBadCase(rv);
        }
        free(*s);
    }
    vectorPoolPut(ptr_pool, v);
}


/*
 *  *****  Sensors  ****************************************************
 */


static void
sensor_end(
    void)
{
    if (!sensor) {
        skpcParseErr("No active sensor in %s statement", pcscan_clause);
        goto END;
    }

    if (defn_errors) {
        goto END;
    }

    if (skpcSensorVerify(sensor, extra_sensor_verify_fn)) {
        skpcParseErr("Unable to verify sensor '%s'",
                     skpcSensorGetName(sensor));
        ++defn_errors;
        goto END;
    }

    /* Sensor is valid and now owned by probeconf.  Set to NULL so it
     * doesn't get free()'ed. */
    sensor = NULL;

  END:
    if (defn_errors) {
        skAppPrintErr("Encountered %d error%s while processing sensor '%s'",
                      defn_errors, ((defn_errors == 1) ? "" : "s"),
                      (sensor ? skpcSensorGetName(sensor) : ""));
        pcscan_errors += defn_errors;
        defn_errors = 0;
    }
    if (sensor) {
        skpcSensorDestroy(&sensor);
        sensor = NULL;
    }
}


/* Begin a new sensor by setting the memory of the global probe_attr_t
 * into its initial state. */
static void
sensor_begin(
    char               *sensor_name)
{
    const char *dummy_name = "<ERROR>";

    if (sensor) {
        skpcParseErr("Found active sensor in %s statement", pcscan_clause);
        skpcSensorDestroy(&sensor);
        sensor = NULL;
    }
    defn_errors = 0;

    if (skpcSensorCreate(&sensor)) {
        skpcParseErr("Fatal: Unable to create sensor");
        exit(EXIT_FAILURE);
    }

    /* sensor_name will only be NULL on bad input from user */
    if (sensor_name == NULL) {
        skpcParseErr("%s requires a sensor name", pcscan_clause);
        ++defn_errors;
        skpcSensorSetName(sensor, dummy_name);
    } else {
#if 0
        if (skpcSensorLookupByName(sensor_name)) {
            skpcParseErr("A sensor named '%s' already exists", sensor_name);
            ++defn_errors;
        }
#endif

        if (skpcSensorSetName(sensor, sensor_name)) {
            skpcParseErr("Error setting sensor name to %s", sensor_name);
            ++defn_errors;
        }

        if (SK_INVALID_SENSOR == skpcSensorGetID(sensor)) {
            skpcParseErr("There is no known sensor named %s", sensor_name);
            ++defn_errors;
        }

        free(sensor_name);
    }
}


/*
 *  sensor_isp_ip(list);
 *
 *    Set the isp-ip's on the global sensor to 'list'.
 */
static void
sensor_isp_ip(
    sk_vector_t        *v)
{
    sk_vector_t *nl;
    size_t count = skVectorGetCount(v);
    size_t i;
    uint32_t ip;
    char **s;

    /* error on overwrite */
    if (skpcSensorGetIspIps(sensor, NULL) != 0) {
        skpcParseErr("Attempt to overwrite previous %s value for sensor '%s'",
                     pcscan_clause, skpcSensorGetName(sensor));
        ++defn_errors;
        vectorPoolPut(ptr_pool, v);
        return;
    }

    nl = vectorPoolGet(u32_pool);
    if (nl == NULL) {
        skpcParseErr("Allocation error near %s", pcscan_clause);
        ++defn_errors;
        return;
    }

    /* convert string list to a list of numerical IPs */
    for (i = 0; i < count; ++i) {
        s = (char**)skVectorGetValuePointer(v, i);
        if (parse_ip_addr(*s, &ip)) {
            ++defn_errors;
        }
        skVectorAppendValue(nl, &ip);
    }
    vectorPoolPut(ptr_pool, v);

    skpcSensorSetIspIps(sensor, nl);
    vectorPoolPut(u32_pool, nl);
}


/*
 *  sensor_interface(name, list);
 *
 *    Set the interface list for the network whose name is 'name' on
 *    the global sensor to 'list'.
 *
 *    If 'list' is NULL, set the interface list to all the indexes NOT
 *    listed on other interfaces---set it to the 'remainder' of the
 *    interfaces.
 */
static void
sensor_interface(
    char               *name,
    sk_vector_t        *v)
{
    const skpc_network_t *network = NULL;
    const size_t count = (v ? skVectorGetCount(v) : 0);
    skpc_group_t *g;
    char **s;
    size_t i;

    if (name == NULL) {
        skpcParseErr("Interface list '%s' gives a NULL name", pcscan_clause);
        skAbort();
    }

    /* convert the name to a network */
    network = skpcNetworkLookupByName(name);
    if (network == NULL) {
        skpcParseErr(("Cannot set %s for sensor '%s' because\n"
                      "\tthe '%s' network is not defined"),
                     pcscan_clause, skpcSensorGetName(sensor), name);
        ++defn_errors;
        goto END;
    }

    /* NULL vector indicates want to set network to 'remainder' */
    if (v == NULL) {
        if (skpcSensorSetToRemainderInterfaces(sensor, network->id)) {
            ++defn_errors;
        }
    } else {
        /* determine if we are using a single existing group */
        if (1 == count) {
            s = (char**)skVectorGetValuePointer(v, 0);
            if ('@' == **s) {
                g = get_group((*s) + 1, SKPC_GROUP_INTERFACE);
                if (NULL == g) {
                    goto END;
                }
                if (skpcSensorSetInterfaces(sensor, network->id, g)) {
                    ++defn_errors;
                }
                goto END;
            }
        }

        /* not a single group, so need to create a new group */
        if (skpcGroupCreate(&g)) {
            skpcParseErr("Allocation error near %s", pcscan_clause);
            ++defn_errors;
            goto END;
        }
        skpcGroupSetType(g, SKPC_GROUP_INTERFACE);

        /* parse the strings and add them to the group */
        if (add_values_to_group(g, v, SKPC_GROUP_INTERFACE)) {
            v = NULL;
            goto END;
        }
        v = NULL;

        /* add the group to the sensor */
        if (skpcGroupFreeze(g)) {
            ++defn_errors;
            goto END;
        }
        if (skpcSensorSetInterfaces(sensor, network->id, g)) {
            ++defn_errors;
        }
    }

  END:
    if (name) {
        free(name);
    }
    if (v) {
        for (i = 0; i < count; ++i) {
            s = (char**)skVectorGetValuePointer(v, i);
            free(*s);
        }
        vectorPoolPut(ptr_pool, v);
    }
}


/*
 *  sensor_ipblock(name, ip_list, negated);
 *
 *    When 'ip_list' is NULL, set a flag for the network 'name' noting
 *    that its ipblock list should be set to any IP addresses not
 *    covered by other IP blocks; ie., the remaining ipblocks.
 *
 *    When 'negated' is 0, set the ipblocks for the 'name' network of
 *    the global sensor to 'ip_list'.
 *
 *    When negated is non-zero, set the ipblocks for the 'name'
 *    network of the global sensor to the inverse of 'ip_list'.
 */
static void
sensor_ipblock(
    char               *name,
    sk_vector_t        *v,
    int                 negated)
{
    const size_t count = (v ? skVectorGetCount(v) : 0);
    const skpc_network_t *network = NULL;
    skpc_group_t *g;
    size_t i;
    char **s;

    if (name == NULL) {
        skpcParseErr("IP Block list '%s' gives a NULL name", pcscan_clause);
        skAbort();
    }

    /* convert the name to a network */
    network = skpcNetworkLookupByName(name);
    if (network == NULL) {
        skpcParseErr(("Cannot set %s for sensor '%s' because\n"
                      "\tthe '%s' network is not defined"),
                     pcscan_clause, skpcSensorGetName(sensor), name);
        ++defn_errors;
        goto END;
    }

    /* NULL vector indicates want to set network to 'remainder' */
    if (v == NULL) {
        if (skpcSensorSetToRemainderIpBlocks(sensor, network->id)) {
            ++defn_errors;
        }
    } else {
        /* determine if we are using a single existing group */
        if (1 == count) {
            s = (char**)skVectorGetValuePointer(v, 0);
            if ('@' == **s) {
                g = get_group((*s)+1, SKPC_GROUP_IPBLOCK);
                if (NULL == g) {
                    goto END;
                }
                if (skpcSensorSetIpBlocks(sensor, network->id, g, negated)) {
                    ++defn_errors;
                }
                goto END;
            }
        }

        /* not a single group, so need to create a new group */
        if (skpcGroupCreate(&g)) {
            skpcParseErr("Allocation error near %s", pcscan_clause);
            ++defn_errors;
            goto END;
        }
        skpcGroupSetType(g, SKPC_GROUP_IPBLOCK);

        /* parse the strings and add them to the group */
        if (add_values_to_group(g, v, SKPC_GROUP_IPBLOCK)) {
            v = NULL;
            goto END;
        }
        v = NULL;

        /* add the group to the sensor */
        if (skpcGroupFreeze(g)) {
            ++defn_errors;
            goto END;
        }
        if (skpcSensorSetIpBlocks(sensor, network->id, g, negated)) {
            ++defn_errors;
            goto END;
        }
    }

  END:
    if (name) {
        free(name);
    }
    if (v) {
        for (i = 0; i < count; ++i) {
            s = (char**)skVectorGetValuePointer(v, i);
            free(*s);
        }
        vectorPoolPut(ptr_pool, v);
    }
}


static void
sensor_filter(
    skpc_filter_t       filter,
    sk_vector_t        *v)
{
    const size_t count = (v ? skVectorGetCount(v) : 0);
    skpc_group_t *g;
    skpc_group_type_t g_type;
    size_t i;
    char **s;

    if (count < 1) {
        skpcParseErr("Missing arguments for %s on sensor '%s'",
                     pcscan_clause, skpcSensorGetName(sensor));
        ++defn_errors;
        goto END;
    }

    /* get the type of data to expect */
    g_type = (filter.f_wildcard ? SKPC_GROUP_IPBLOCK : SKPC_GROUP_INTERFACE);

    /* determine if we are using a single existing group */
    if (1 == count) {
        s = (char**)skVectorGetValuePointer(v, 0);
        if ('@' == **s) {
            g = get_group((*s) + 1, g_type);
            if (NULL == g) {
                goto END;
            }
            if (skpcSensorAddFilter(sensor, g, filter.f_type,
                                    filter.f_discwhen, filter.f_wildcard))
            {
                ++defn_errors;
            }
            goto END;
        }
    }

    /* not a single group, so need to create a new group */
    if (skpcGroupCreate(&g)) {
        skpcParseErr("Allocation error near %s", pcscan_clause);
        ++defn_errors;
        goto END;
    }
    skpcGroupSetType(g, g_type);

    /* parse the strings in 'v' and add them to the group */
    if (add_values_to_group(g, v, g_type)) {
        v = NULL;
        goto END;
    }
    v = NULL;

    /* add the group to the filter */
    if (skpcGroupFreeze(g)) {
        ++defn_errors;
        goto END;
    }
    if (skpcSensorAddFilter(sensor, g, filter.f_type, filter.f_discwhen,
                            filter.f_wildcard))
    {
        ++defn_errors;
    }

  END:
    if (v) {
        for (i = 0; i < count; ++i) {
            s = (char**)skVectorGetValuePointer(v, i);
            free(*s);
        }
        vectorPoolPut(ptr_pool, v);
    }
}


static void
sensor_network(
    skpc_direction_t    direction,
    char               *name)
{
    const skpc_network_t *network = NULL;

    if (name == NULL) {
        skpcParseErr("Missing network name in %s on sensor '%s'",
                     pcscan_clause, skpcSensorGetName(sensor));
        ++defn_errors;
        goto END;
    }

    /* convert the name to a network */
    network = skpcNetworkLookupByName(name);
    if (network == NULL) {
        skpcParseErr(("Cannot set %s for sensor '%s' because\n"
                      "\tthe '%s' network is not defined"),
                     pcscan_clause, skpcSensorGetName(sensor), name);
        ++defn_errors;
        goto END;
    }

    if (skpcSensorSetNetwork(sensor, network->id, direction)) {
        skpcParseErr("Cannot set %s for sensor '%s' to %s",
                     pcscan_clause, skpcSensorGetName(sensor), name);
        ++defn_errors;
        goto END;
    }

  END:
    if (name) {
        free(name);
    }
}


static void
sensor_probes(
    char               *probe_type,
    sk_vector_t        *v)
{
    sk_vector_t *pl;
    size_t i = 0;
    char **s;
    const skpc_probe_t *p;
    skpc_probetype_t t;

    /* get a vector to use for the probe objects */
    pl = vectorPoolGet(ptr_pool);

    /* get the probe-type */
    t = skpcProbetypeNameToEnum(probe_type);
    if (t == PROBE_ENUM_INVALID) {
        skpcParseErr("Do not recognize probe type '%s'", probe_type);
        ++defn_errors;
        goto END;
    }

    while (NULL != (s = (char**)skVectorGetValuePointer(v, i))) {
        ++i;
        p = skpcProbeLookupByName(*s);
        if (p) {
            if (skpcProbeGetType(p) != t) {
                skpcParseErr("Attempt to use %s probe '%s' in a %s statement",
                             skpcProbetypeEnumtoName(skpcProbeGetType(p)),
                             *s, pcscan_clause);
                ++defn_errors;
            }
        } else {
            /* Create a new probe with the specified name and type */
            skpc_probe_t *new_probe;
            if (skpcProbeCreate(&new_probe)) {
                skpcParseErr("Fatal: Unable to create ephemeral probe");
                exit(EXIT_FAILURE);
            }
            if (skpcProbeSetName(new_probe, *s)) {
                skpcParseErr("Error setting ephemeral probe name to %s", *s);
                ++defn_errors;
                goto END;
            }
            if (skpcProbeSetType(new_probe, t)) {
                skpcParseErr("Error setting ephemeral probe type to %s",
                             probe_type);
                ++defn_errors;
                goto END;
            }
            if (skpcProbeVerify(new_probe, 1)) {
                skpcParseErr("Error verifying ephemeral probe '%s'",
                             *s);
                ++defn_errors;
                goto END;
            }
            p = skpcProbeLookupByName(*s);
            if (p == NULL) {
                skpcParseErr("Cannot find newly created ephemeral probe '%s'",
                             *s);
                skAbort();
            }
        }
        skVectorAppendValue(pl, &p);
        free(*s);
    }

    if (skpcSensorSetProbes(sensor, pl)) {
        ++defn_errors;
    }

  END:
    free(probe_type);
    while (NULL != (s = (char**)skVectorGetValuePointer(v, i))) {
        ++i;
        free(*s);
    }
    vectorPoolPut(ptr_pool, v);
    vectorPoolPut(ptr_pool, pl);
}


/*
 *  *****  Groups  ****************************************************
 */


static void
group_end(
    void)
{
    if (!group) {
        skpcParseErr("No active group in %s statement", pcscan_clause);
        goto END;
    }

    if (defn_errors) {
        goto END;
    }

    if (skpcGroupFreeze(group)) {
        skpcParseErr("Unable to freeze group '%s'",
                     skpcGroupGetName(group));
        ++defn_errors;
        goto END;
    }

    /* Group is valid and now owned by probeconf.  Set to NULL so it
     * doesn't get free()'ed. */
    group = NULL;

  END:
    if (defn_errors) {
        skAppPrintErr("Encountered %d error%s while processing group '%s'",
                      defn_errors, ((defn_errors == 1) ? "" : "s"),
                      (group ? skpcGroupGetName(group) : ""));
        pcscan_errors += defn_errors;
        defn_errors = 0;
    }
    if (group) {
        skpcGroupDestroy(&group);
        group = NULL;
    }
}


/* Begin a new group by setting the memory of the global probe_attr_t
 * into its initial state. */
static void
group_begin(
    char               *group_name)
{
    const char *dummy_name = "<ERROR>";

    if (group) {
        skpcParseErr("Found active group in %s statement", pcscan_clause);
        skpcGroupDestroy(&group);
        group = NULL;
    }
    defn_errors = 0;

    if (skpcGroupCreate(&group)) {
        skpcParseErr("Fatal: Unable to create group");
        exit(EXIT_FAILURE);
    }

    /* group_name will only be NULL on bad input from user */
    if (group_name == NULL) {
        skpcParseErr("%s requires a group name", pcscan_clause);
        ++defn_errors;
        skpcGroupSetName(group, dummy_name);
    } else {
        if (skpcGroupLookupByName(group_name)) {
            skpcParseErr("A group named '%s' already exists", group_name);
            ++defn_errors;
        }
        if (skpcGroupSetName(group, group_name)) {
            skpcParseErr("Error setting group name to %s", group_name);
            ++defn_errors;
        }
        free(group_name);
    }
}


/*
 *  group_add_data(v, g_type);
 *
 *   Verify that the global group has a type of 'g_type'.  If so,
 *   parse the string values in 'v' and add the values to the global
 *   group.
 *
 *   If the global group's type is not set, the value to 'g_type' and
 *   append the values.
 *
 *   Used by 'stmt_group_interfaces' and 'stmt_group_ipblocks'
 */
static void
group_add_data(
    sk_vector_t        *v,
    skpc_group_type_t   g_type)
{
    const char *g_type_str = "unknown data";
    size_t i = 0;
    char **s;

    switch (skpcGroupGetType(group)) {
      case SKPC_GROUP_UNSET:
        skpcGroupSetType(group, g_type);
        break;
      case SKPC_GROUP_INTERFACE:
        g_type_str = "interface values";
        break;
      case SKPC_GROUP_IPBLOCK:
        g_type_str = "ipblocks";
        break;
    }

    if (g_type != skpcGroupGetType(group)) {
        skpcParseErr(("Cannot add %s to group because\n"
                      "\tthe group already contains %s"),
                     pcscan_clause, g_type_str);
        ++defn_errors;
        goto END;
    }

    add_values_to_group(group, v, g_type);
    v = NULL;

  END:
    if (v) {
        i = skVectorGetCount(v);
        while (i > 0) {
            --i;
            s = (char**)skVectorGetValuePointer(v, i);
            free(*s);
        }
        vectorPoolPut(ptr_pool, v);
    }
}


static int
add_values_to_group(
    skpc_group_t       *g,
    sk_vector_t        *v,
    skpc_group_type_t   g_type)
{
    const size_t count = (v ? skVectorGetCount(v) : 0);
    skpc_group_t *named_group;
    vector_pool_t *pool;
    sk_vector_t *vec = NULL;
    char **s;
    size_t i = 0;
    uint32_t n;
    skIPWildcard_t *ipwild;
    int rv = -1;

    /* determine the vector pool to use for the parsed values */
    if (SKPC_GROUP_INTERFACE == g_type) {
        /* parse numbers and/or groups */
        pool = u32_pool;
    } else {
        assert(SKPC_GROUP_IPBLOCK == g_type);

        /* parse ipblocks and/or groups */
        pool = ptr_pool;
    }

    /* get a vector from the pool */
    vec = vectorPoolGet(pool);
    if (vec == NULL) {
        skpcParseErr("Allocation error near %s", pcscan_clause);
        ++defn_errors;
        goto END;
    }

    /* process the strings in the vector 'v' */
    for (i = 0; i < count; ++i) {
        s = (char**)skVectorGetValuePointer(v, i);

        /* is this a group? */
        if ('@' == **s) {
            named_group = get_group((*s)+1, g_type);
            free(*s);
            if (NULL == named_group) {
                ++i;
                goto END;
            }
            if (skpcGroupAddGroup(g, named_group)) {
                ++defn_errors;
                ++i;
                goto END;
            }
        } else if (g_type == SKPC_GROUP_IPBLOCK) {
            assert(pool == ptr_pool);
            ipwild = parse_wildcard_addr(*s);
            if (ipwild == NULL) {
                ++defn_errors;
                ++i;
                goto END;
            }
            skVectorAppendValue(vec, &ipwild);
        } else {
            assert(g_type == SKPC_GROUP_INTERFACE);
            assert(pool == u32_pool);
            n = parse_int_u16(*s);
            if (n == UINT16_NO_VALUE) {
                ++defn_errors;
                ++i;
                goto END;
            }
            skVectorAppendValue(vec, &n);
        }
    }

    /* add values to the group */
    if (skpcGroupAddValues(g, vec)) {
        ++defn_errors;
    }

    rv = 0;

  END:
    for ( ; i < count; ++i) {
        s = (char**)skVectorGetValuePointer(v, i);
        free(*s);
    }
    if (v) {
        vectorPoolPut(ptr_pool, v);
    }
    if (vec) {
        vectorPoolPut(pool, vec);
    }
    return rv;
}


static skpc_group_t *
get_group(
    const char         *g_name,
    skpc_group_type_t   g_type)
{
    skpc_group_t *g;

    g = skpcGroupLookupByName(g_name);
    if (!g) {
        skpcParseErr("Error in %s: group '%s' is not defined",
                     pcscan_clause, g_name);
        ++defn_errors;
        return NULL;
    }
    if (skpcGroupGetType(g) != g_type) {
        skpcParseErr(("Error in %s: the '%s' group does not contain %s"),
                     pcscan_clause, g_name,
                     ((g_type == SKPC_GROUP_IPBLOCK)
                      ? "ipblocks"
                      : "interfaces"));
        ++defn_errors;
        return NULL;
    }
    return g;
}


/*
 *  *****  Parsing Utilities  ******************************************
 */


/*
 *  val = parse_int_u16(s);
 *
 *    Parse 's' as a integer from 0 to 0xFFFF inclusive.  Returns the
 *    value on success.  Prints an error and returns UINT16_NO_VALUE
 *    if parsing is unsuccessful or value is out of range.
 */
static uint32_t
parse_int_u16(
    char               *s)
{
    uint32_t num;
    int rv;

    rv = skStringParseUint32(&num, s, 0, 0xFFFF);
    if (rv) {
        skpcParseErr("Invalid %s '%s': %s",
                     pcscan_clause, s, skStringParseStrerror(rv));
        num = UINT16_NO_VALUE;
    }

    free(s);
    return num;
}


/*
 *  ok = vectorSingleString(v, &s);
 *
 *    If the vector 'v' contains a single value, set 's' to point at
 *    that value, put 'v' into the vector pool, and return 0.
 *
 *    Otherwise, print an error message, increment the error count,
 *    destroy all the strings in 'v', put 'v' into the vector pool,
 *    and return -1.
 */
static int
vectorSingleString(
    sk_vector_t        *v,
    char              **s)
{
    int rv = 0;

    if (1 == skVectorGetCount(v)) {
        skVectorGetValue(s, v, 0);
    } else {
        size_t i = 0;
        while (NULL != (s = (char**)skVectorGetValuePointer(v, i))) {
            ++i;
            free(*s);
        }
        skpcParseErr("The %s clause takes a single argument", pcscan_clause);
        ++defn_errors;
        rv = -1;
    }

    vectorPoolPut(ptr_pool, v);
    return rv;
}


/*
 *  ipwild = parse_wildcard_addr(ip);
 *
 *    Parse 'ip' as an IP address block in SiLK wildcard notation.
 *    Because the scanner does not allow comma as part of an ID, we
 *    will never see things like  "10.20.30.40,50".
 *
 *    Return the set of ips as an skIPWildcard_t*, or NULL on error.
 */
static skIPWildcard_t *
parse_wildcard_addr(
    char               *s)
{
    skIPWildcard_t *ipwild;
    int rv;

    ipwild = (skIPWildcard_t*)malloc(sizeof(skIPWildcard_t));
    if (ipwild) {
        rv = skStringParseIPWildcard(ipwild, s);
        if (rv) {
            skpcParseErr("Invalid IP address block '%s': %s",
                         s, skStringParseStrerror(rv));
            free(ipwild);
            ipwild = NULL;
        }
    }

    free(s);
    return ipwild;
}


/*
 *  ok = parse_ip_addr(ip_string, ip_val);
 *
 *    Parse 'ip_string' as an IP address and put result into 'ip_val'.
 *    Return 0 on success, -1 on failure.
 */
static int
parse_ip_addr(
    char               *s,
    uint32_t           *ip)
{
    skipaddr_t addr;
    int rv;

    rv = skStringParseIP(&addr, s);
    if (rv) {
        skpcParseErr("Invalid IP addresses '%s': %s",
                     s, skStringParseStrerror(rv));
        free(s);
        return -1;
    }
#if SK_ENABLE_IPV6
    if (skipaddrIsV6(&addr)) {
        skpcParseErr("Invalid IP address '%s': IPv6 addresses not supported",
                     s);
        free(s);
        return -1;
    }
#endif /* SK_ENABLE_IPV6 */

    free(s);
    *ip = skipaddrGetV4(&addr);
    return 0;
}


/*
 *  new_flags = log_flags_add_flag(old_flags, flag);
 *
 *    Add 'flag' to the 'old_flags' and return the new flags.  Warn if
 *    either value is explicitly "NONE".
 */
static uint32_t
log_flags_add_flag(
    uint32_t            old_flags,
    uint32_t            flag)
{
    if ((old_flags == SOURCE_LOG_NONE) || (flag == SOURCE_LOG_NONE)) {
        skpcParseErr(("Cannot mix %s 'none' with other values on probe '%s'"),
                     pcscan_clause, skpcProbeGetName(probe));
        ++defn_errors;
    }

    return (old_flags | flag);
}


/*
 *  flag = parse_log_flag(s);
 *
 *    Parse the string 's' as a log flag.
 */
static uint32_t
parse_log_flag(
    char               *s)
{
    int rv = UINT16_NO_VALUE;

    if (!s || !*s) {
        skpcParseErr("Missing value for %s on probe '%s'",
                     pcscan_clause, skpcProbeGetName(probe));
        ++defn_errors;
    } else if (0 == strcmp(s, "all")) {
        rv = SOURCE_LOG_ALL;
    } else if (0 == strcmp(s, "bad")) {
        rv = SOURCE_LOG_BAD;
    } else if (0 == strcmp(s, "missing")) {
        rv = SOURCE_LOG_MISSING;
    } else if (0 == strcmp(s, "none")) {
        rv = SOURCE_LOG_NONE;
    } else if (0 == strcmp(s, "sampling")) {
        rv = SOURCE_LOG_SAMPLING;
    } else if (0 == strcmp(s, "firewall-event")) {
        rv = SOURCE_LOG_FIREWALL;
    } else {
        skpcParseErr("Do not recognize %s value '%s' on probe '%s'",
                     pcscan_clause, s, skpcProbeGetName(probe));
        ++defn_errors;
    }

    if (s) {
        free(s);
    }
    return rv;
}


int
yyerror(
    char        UNUSED(*s))
{
    return 0;
}


int
skpcParseSetup(
    void)
{
    memset(ptr_pool, 0, sizeof(vector_pool_t));
    ptr_pool->element_size = sizeof(char*);

    memset(u32_pool, 0, sizeof(vector_pool_t));
    u32_pool->element_size = sizeof(uint32_t);

    return 0;
}


void
skpcParseTeardown(
    void)
{
    if (probe) {
        ++defn_errors;
        skpcParseErr("Missing \"end probe\" statement");
        skpcProbeDestroy(&probe);
        probe = NULL;
    }
    if (sensor) {
        ++defn_errors;
        skpcParseErr("Missing \"end sensor\" statement");
        skpcSensorDestroy(&sensor);
        sensor = NULL;
    }
    if (group) {
        ++defn_errors;
        skpcParseErr("Missing \"end group\" statement");
        skpcGroupDestroy(&group);
        group = NULL;
    }

    pcscan_errors += defn_errors;
    vectorPoolEmpty(ptr_pool);
    vectorPoolEmpty(u32_pool);
}


/*
** Local variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
