/*
** Copyright (C) 2010-2014 by Carnegie Mellon University.
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

#include <silk/silk.h>

RCSIDENT("$SiLK: ipafilter.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/rwrec.h>
#include <silk/skipaddr.h>
#include <silk/skplugin.h>
#include <silk/skstream.h>
#include "rwipa.h"


/* Plugin protocol version */
#define PLUGIN_API_VERSION_MAJOR 1
#define PLUGIN_API_VERSION_MINOR 0

/* LOCAL VARIABLES */

static IPAContext *ipa;
static char       *ipa_db_uri = NULL;

static skipset_t *src_pass_set = NULL;
static skipset_t *dst_pass_set = NULL;
static skipset_t *any_pass_set = NULL;

static const char *ipa_src_expr = NULL;
static const char *ipa_dst_expr = NULL;
static const char *ipa_any_expr = NULL;
/* static uint8_t ipa_lazy_load   = 0; */

static const char *optname_ipa_src_expr  = "ipa-src-expr";
static const char *optname_ipa_dst_expr  = "ipa-dst-expr";
static const char *optname_ipa_any_expr  = "ipa-any-expr";

/* Set to TRUE when any of our command line options is specified */
static int ipafilter_enabled = FALSE;

/* static char *optname_ipa_lazy_load = "ipa-lazy-load"; */

static void ipafilter_preload_set(skipset_t *set);

static skplugin_err_t
ipafilter_init(
    void               *cbdata);

static skplugin_err_t
ipafilter_filter(
    const rwRec        *rwrec,
    void        UNUSED(*data),
    void              **extra);

static skplugin_err_t
ipafilter_filter_preloaded(
    const rwRec        *rec,
    void               *data);

#if 0
static skplugin_err_t
ipafilter_filter_lazy(
    const rwRec        *rec,
    void               *data);
#endif

static skplugin_err_t
ipafilter_cleanup(
    void               *cbdata);
/*
 *     skplugin_err_t SKPLUGIN_SETUP_FN(
 *       uint16_t  major_version,
 *       uint16_t  minor_version,
 *      void    *data);
 */

/* Register all the fields and functions with the plugin library */
static skplugin_err_t
ipafilter_register(
    void)
{
    skplugin_callbacks_t regdata;

    if (ipafilter_enabled) {
        return SKPLUGIN_OK;
    }

    ipafilter_enabled = TRUE;

    memset(&regdata, 0, sizeof(regdata));
    regdata.init    = ipafilter_init;
    regdata.cleanup = ipafilter_cleanup;
    regdata.filter  = ipafilter_filter;

    /* Set the functions for rwfilter */
    return skpinRegFilter(NULL, &regdata, NULL);
} /* ipafilter_register */


static skplugin_err_t
ipafilter_handle_src_expr(
    const char         *opt_arg,
    void        UNUSED(*cbdata))
{
    if (ipa_src_expr != NULL) {
        skAppPrintErr("Invalid %s: Switch used multiple times",
                      optname_ipa_src_expr);
        return SKPLUGIN_ERR;
    }
    ipa_src_expr = opt_arg;

    return ipafilter_register();
} /* ipafilter_handle_time */

static skplugin_err_t
ipafilter_handle_dst_expr(
    const char         *opt_arg,
    void        UNUSED(*cbdata))
{
    if (ipa_dst_expr != NULL) {
        skAppPrintErr("Invalid %s: Switch used multiple times",
                      optname_ipa_dst_expr);
        return SKPLUGIN_ERR;
    }
    ipa_dst_expr = opt_arg;

    return ipafilter_register();
} /* ipafilter_handle_time */

static skplugin_err_t
ipafilter_handle_any_expr(
    const char         *opt_arg,
    void        UNUSED(*cbdata))
{
    if (ipa_dst_expr != NULL) {
        skAppPrintErr("Invalid %s: Switch used multiple times",
                      optname_ipa_any_expr);
        return SKPLUGIN_ERR;
    }
    ipa_any_expr = opt_arg;

    return ipafilter_register();
} /* ipafilter_handle_time */


#if 0
static skplugin_err_t
ipafilter_handle_lazy_load(
    const char  UNUSED(*opt_arg),
    void        UNUSED(*cbdata))
{
    ipa_lazy_load = 1;
    return SKPLUGIN_OK;
} /* ipafilter_handle_lazy_load */
#endif

/* Public plugin entry point */
skplugin_err_t
SKPLUGIN_SETUP_FN(
    uint16_t            major_version,
    uint16_t            minor_version,
    void        UNUSED(*data))
{
    skplugin_err_t rv;

    /* Check API version */
    rv = skpinSimpleCheckVersion(major_version, minor_version,
                                 PLUGIN_API_VERSION_MAJOR,
                                 PLUGIN_API_VERSION_MINOR,
                                 skAppPrintErr);
    if (rv != SKPLUGIN_OK) {
        return rv;
    }

    skpinRegOption2(optname_ipa_src_expr,
                    REQUIRED_ARG,
                    ("IPA query expression to be applied to the source\n"
                     "\tIP address"),
                    NULL,
                    ipafilter_handle_src_expr,
                    NULL,
                    1, SKPLUGIN_APP_FILTER);

    skpinRegOption2(optname_ipa_dst_expr,
                    REQUIRED_ARG,
                    ("IPA query expression to be applied to the destination\n"
                     "\tIP address"),
                    NULL,
                    ipafilter_handle_dst_expr,
                    NULL,
                    1, SKPLUGIN_APP_FILTER);

    skpinRegOption2(optname_ipa_any_expr,
                    REQUIRED_ARG,
                    ("IPA query expression to be applied to the source or\n"
                     "\tdestination IP address"),
                    NULL, ipafilter_handle_any_expr,
                    NULL,
                    1, SKPLUGIN_APP_FILTER);

#if 0
    skpinRegOption2(optname_ipa_lazy_load,
                    NO_ARG,
                    ("IPA lazy load"),
                    NULL,
                    ipafilter_handle_lazy_load,
                    NULL,
                    1, SKPLUGIN_APP_FILTER);
#endif

    return SKPLUGIN_OK;
} /* SKPLUGIN_SETUP_FN */



static skplugin_err_t
ipafilter_init(
    void        UNUSED(*cbdata))
{
    skplugin_err_t err = SKPLUGIN_ERR;
    int rv;

    if (! ipafilter_enabled) {
        return SKPLUGIN_OK;
    }

    /* We are not thread safe */
    skpinSetThreadNonSafe();

    ipa_db_uri = get_ipa_config();

    if (ipa_db_uri == NULL) {
        skAppPrintErr("Could not get IPA configuration");
        err = SKPLUGIN_ERR;
        goto done;
    }

    if (ipa_create_context(&ipa, ipa_db_uri, NULL) != IPA_OK) {
        skAppPrintErr("Could not create IPA context");
        err = SKPLUGIN_ERR;
        goto done;
    }

    ipa->verbose = FALSE;

    if (ipa_src_expr) {
        rv = ipa_parse_query(ipa, (char*)ipa_src_expr);
        switch (rv) {
          case IPA_OK:
            break;
          case IPA_ERR_NOTFOUND:
            skAppPrintErr("Dataset not found for given name and time");
            err = SKPLUGIN_ERR;
            goto done;
          default:
            skAppPrintErr("IPA error retrieving dataset");
            err = SKPLUGIN_ERR;
            goto done;
        } /* switch */

        if (skIPSetCreate(&src_pass_set, 0)) {
            skAppPrintErr("error creating src pass set");
            err = SKPLUGIN_ERR;
            goto done;
        }
        ipafilter_preload_set(src_pass_set);
    }
    if (ipa_dst_expr) {
        rv = ipa_parse_query(ipa, (char*)ipa_dst_expr);
        switch (rv) {
          case IPA_OK:
            break;
          case IPA_ERR_NOTFOUND:
            skAppPrintErr("Dataset not found for given name and time");
            err = SKPLUGIN_ERR;
            goto done;
          default:
            skAppPrintErr("IPA error retrieving dataset");
            err = SKPLUGIN_ERR;
            goto done;
        } /* switch */

        if (skIPSetCreate(&dst_pass_set, 0)) {
            skAppPrintErr("error creating dst pass set");
            err = SKPLUGIN_ERR;
            goto done;
        }
        ipafilter_preload_set(dst_pass_set);

    }
    if (ipa_any_expr) {
        rv = ipa_parse_query(ipa, (char*)ipa_any_expr);
        switch (rv) {
          case IPA_OK:
            break;
          case IPA_ERR_NOTFOUND:
            skAppPrintErr("Dataset not found for given name and time");
            err = SKPLUGIN_ERR;
            goto done;
          default:
            skAppPrintErr("IPA error retrieving dataset");
            err = SKPLUGIN_ERR;
            goto done;
        } /* switch */
        if (skIPSetCreate(&any_pass_set, 0)) {
            skAppPrintErr("Error creating any pass set");
            err = SKPLUGIN_ERR;
            goto done;
        }
        ipafilter_preload_set(any_pass_set);
    }

    err = SKPLUGIN_OK;

  done:
    if (err != SKPLUGIN_OK) {
        if (ipa) {
            ipa_destroy_context(&ipa);
        }
    }
    return err;
} /* ipafilter_init */


void
ipafilter_preload_set(
    skipset_t          *set)
{
    skIPWildcard_t ipwild;
    int rv;
    IPAAssoc   assoc;
    char *cp = NULL;
    skipaddr_t begin, end;

    assert(set);

    while (!ipa_get_next_assoc(ipa, &assoc)) {
        /* SiLK wildcards don't do a.b.c.d-e.f.g.h IP range format, so we have
           to check for it here */
        if (!strchr(assoc.range, '-')) {
            /* The range should be grokkable */
            rv = skStringParseIPWildcard(&ipwild, assoc.range);
            if (rv) {
                /* error */
                skAppPrintErr("Invalid IP string for wildcard %s: %s",
                              assoc.range, skStringParseStrerror(rv));
                return;
            }
            skIPSetInsertIPWildcard(set, &ipwild);

        } else {
            /* For non-CIDR ranges, we need to manually add each address. */
            cp = strchr(assoc.range, '-');
            /* We checked ealier to make sure the hyphen was there */
            *cp = '\0';
            rv =  skStringParseIP(&begin, assoc.range);
            if (rv) {
                skAppPrintErr("Invalid IP string in IP range: %s, %s",
                              assoc.range, skStringParseStrerror(rv));
            }
            *cp = '-';
            cp++;
            rv =  skStringParseIP(&end, cp);
            if (rv) {
                skAppPrintErr("Invalid IP string in IP range: %s, %s",
                              assoc.range, skStringParseStrerror(rv));
            }
            skIPSetInsertRange(set, &begin, &end);
        }
    }
} /* ipafilter_preload_set */


/* Filter based on an rwrec */
static skplugin_err_t
ipafilter_filter(
    const rwRec            *rwrec,
    void            UNUSED(*data),
    void           UNUSED(**extra))
{
    return ipafilter_filter_preloaded(rwrec, data);
} /* ipafilter_filter */

static skplugin_err_t
ipafilter_filter_preloaded(
    const rwRec        *rwrec,
    void        UNUSED(*data)
    )
{
    skipaddr_t src, dst;

    rwRecMemGetSIP(rwrec, &src);
    rwRecMemGetDIP(rwrec, &dst);

    if (ipa_src_expr) {
        if (! skIPSetCheckAddress(src_pass_set, &src)) {
            return SKPLUGIN_FILTER_FAIL;
        }
    }

    if (ipa_dst_expr) {
        if (! skIPSetCheckAddress(dst_pass_set, &dst)) {
            return SKPLUGIN_FILTER_FAIL;
        }
    }

    if (ipa_any_expr) {
        if (! (skIPSetCheckAddress(any_pass_set, &src)
               || skIPSetCheckAddress(any_pass_set, &dst))) {
            return SKPLUGIN_FILTER_FAIL;
        }
    }

    return SKPLUGIN_FILTER_PASS;

} /* ipafilter_filter_preloaded */

#if 0
static skplugin_err_t
ipafilter_filter_lazy(
    const rwRec        *rwrec,
    void        UNUSED(*data)
    )
{
    int rv = 0;
    IPAStatus ipa_rv;
    IPAAssoc  assoc;
    uint32_t  addr;
    skIPWildcard_t ipwild;
    skipaddr_t ipaddr;

    addr = rwRecGetSIPv4(rwrec);

    if (skIPSetCheckAddress(pass_set, addr)) {
        /* this address was already passed */
        /* fprintf(stderr, "already passed\n"); */
        return SKPLUGIN_FILTER_PASS;
    } else if (skIPSetCheckAddress(fail_set, addr)) {
        /* this address was already failed */
        /* fprintf(stderr, "already failed\n"); */
        return SKPLUGIN_FILTER_FAIL;
    } else {
        /* haven't seen this address, let's check IPA */
        ipa_rv = ipa_find_assoc(ipa, &assoc, addr, NULL);
        switch (ipa_rv) {
          case IPA_OK:
            /* Add the matching range from IPA to the pass tree */
            rv = skStringParseIPWildcard(&ipwild, assoc.range);
            if (rv) {
                skAppPrintErr("Invalid IP string %s: %s",
                              assoc.range, skStringParseStrerror(rv));
                return SKPLUGIN_ERR;
            }
            skIPTreeAddIPWildcard(pass_set, &ipwild);
            /* fprintf(stderr, "pass: %s\n", num2dot(addr)); */
            return SKPLUGIN_FILTER_PASS;
          case IPA_ERR_NOTFOUND:
            /* Add the failed address to the fail tree */
            /* fprintf(stderr, "fail: %s\n", num2dot(addr)); */
            skIPTreeAddAddress(fail_set, addr);
            return SKPLUGIN_FILTER_FAIL;
          default:
            skAppPrintErr("IPA error finding range in dataset");
            return SKPLUGIN_ERR;
        } /* switch */
    }
}         /* ipafilter_filter_lazy */
#endif /* if 0 */


static skplugin_err_t
ipafilter_cleanup(
    void        UNUSED(*cbdata))
{
    if (src_pass_set) {
        skIPSetDestroy(&src_pass_set);
    }
    if (dst_pass_set) {
        skIPSetDestroy(&dst_pass_set);
    }
    if (any_pass_set) {
        skIPSetDestroy(&any_pass_set);
    }
    return SKPLUGIN_OK;
} /* ipafilter_cleanup */


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
