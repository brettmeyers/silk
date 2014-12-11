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

#include <silk/silk.h>

RCSIDENT("$SiLK: skstringmap-test.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/skstringmap.h>

static void
test_string(
    sk_dllist_t            *name_id_map,
    const char             *user_input,
    sk_stringmap_status_t   desired_parse_status,
    size_t                  desired_count);

static void
test_get_by_name(
    sk_stringmap_t     *name_id_map);

static void
test_get_by_id(
    sk_stringmap_t     *name_id_map);

static const char *status_string(sk_stringmap_status_t st);


int main()
{
    int i;
    const char *s;
    sk_stringmap_status_t err;
    sk_stringmap_t *name_id_map;
    sk_stringmap_entry_t add_entry;
    sk_stringmap_entry_t a_few_ids[] = {
        { "foo", 1, NULL, NULL },
        { "bar", 2, NULL, NULL }
    };

    sk_stringmap_entry_t a_few_more_ids[] = {
        { "baZ", 3, NULL, NULL },
        { "foo", 1, NULL, NULL },
        { "2", 3, NULL, NULL },
        { "3", 4, NULL, NULL },
        { "4", 5, NULL, NULL },
        { "5", 6, NULL, NULL },
        { "bar", 2, NULL, NULL },
        { "food", 4, NULL, NULL },
        { "bar-baz", 5, NULL, NULL },
        { "101", 101, NULL, NULL },
        { "suM", 45, NULL, NULL }
    };

    sk_stringmap_entry_t remove_these_ids[] = {
        { "2", 3, NULL, NULL },
        { "bar", 2, NULL, NULL }
    };

    struct {
        const char *name;
        int         count;
    } parseable[] = {
        {"foo", 1},
        {"foo,bar", 2},
        {",,,foo,,,,bar,,,,baz,,,,", 3},
        {"bar,foo", 2},
        {"foo,2", 2},
        {"2", 1},
        {"2-2", 1},
        {"2,2", 2},
        {"2-3", 2},
        {"2-4", 3},
        {"2-5", 4},
        {"4-5", 2},
        {"3-4", 2},
        {"sum", 1},
        {"suM", 1},
        {"su",  1},
        {"foo,foo,foo", 3},
        {NULL, 0}
    };

    const char *ambiguous[] = {
        "ba",
        "fo",
        NULL
    };

    const char *no_match[] = {
        "a",
        "1",
        "1,2",
        /* big number isn't parsed as a number by itself */
        "75984752347525734798875759887523794753927734927",
        "1-3",
        "2-6",
        "foo-bar",
        "1-3,foo",
        "foo,1-1",
        NULL
    };

    const char *unparseable[] = {
        "2-1",
        "foo,2-1",
        "1jjh-5000",
        "1-2-3-4",
        /* big number as part of a range is parsed as a number */
        "1-75984752347525734798875759887523794753927734927",
        "1--3",
        "2-",
        "-3",
        "5,2-,-3,4",
        NULL
    };


    err = skStringMapCreate(&name_id_map);
    if (err != SKSTRINGMAP_OK) {
        printf("error allocating list\n");
    }

    s = "baz";
    memset(&add_entry, 0, sizeof(add_entry));
    add_entry.name = s;
    add_entry.id = 3;
    err = skStringMapAddEntries(name_id_map, 1, &add_entry);
    if (err != SKSTRINGMAP_OK) {
        printf("error %d (%s) adding '%s' to\n\t",
               err, status_string(err), s);
        skStringMapPrintMap(name_id_map, stdout);
        printf("\n");
    }

    err = skStringMapAddEntries(name_id_map,
                                sizeof(a_few_ids)/sizeof(a_few_ids[0]),
                                a_few_ids);
    if (err != SKSTRINGMAP_OK) {
        printf("error %d (%s) adding list of a_few_ids to\n\t",
               err, status_string(err));
        skStringMapPrintMap(name_id_map, stdout);
        printf("\n");
    }

    s = "2";
    memset(&add_entry, 0, sizeof(add_entry));
    add_entry.name = s;
    add_entry.id = 8;
    err = skStringMapAddEntries(name_id_map, 1, &add_entry);
    if (err != SKSTRINGMAP_OK) {
        printf("error %d (%s) adding '%s' to\n\t",
               err, status_string(err), s);
        skStringMapPrintMap(name_id_map, stdout);
        printf("\n");
    }

    /* try dup */
    memset(&add_entry, 0, sizeof(add_entry));
    add_entry.id = 3;
    err = skStringMapAddEntries(name_id_map, 1, &add_entry);
    if (err != SKSTRINGMAP_OK) {
        printf("correctly got error %d (%s) adding '%s' to\n\t",
               err, status_string(err), s);
        skStringMapPrintMap(name_id_map, stdout);
        printf("\n");
    }

    s = "-cow";
    memset(&add_entry, 0, sizeof(add_entry));
    add_entry.name = s;
    add_entry.id = 6;
    err = skStringMapAddEntries(name_id_map, 1, &add_entry);
    if (err != SKSTRINGMAP_OK) {
        printf("correctly got error %d (%s) adding '%s' to\n\t",
               err, status_string(err), s);
        skStringMapPrintMap(name_id_map, stdout);
        printf("\n");
    }

    s = "35x";
    memset(&add_entry, 0, sizeof(add_entry));
    add_entry.name = s;
    add_entry.id = 7;
    err = skStringMapAddEntries(name_id_map, 1, &add_entry);
    if (err != SKSTRINGMAP_OK) {
        printf("correctly got error %d (%s) adding '%s' to\n\t",
               err, status_string(err), s);
        skStringMapPrintMap(name_id_map, stdout);
        printf("\n");
    }

    /* test removing items */
    printf("list should have "
           "{ \"baz\", \"foo\", \"bar\", \"2\" } \n\t");
    skStringMapPrintMap(name_id_map, stdout);
    printf("\n\n");

    skStringMapRemoveByName(name_id_map, "foo");

    printf("list should have "
           "{ \"baz\", \"bar\", \"2\" }\n\t");
    skStringMapPrintMap(name_id_map, stdout);
    printf("\n\n");

    if (skStringMapRemoveEntries(name_id_map,
                                 (sizeof(remove_these_ids)
                                  / sizeof(remove_these_ids[0])),
                                 remove_these_ids)
        != SKSTRINGMAP_OK)
    {
        printf("error removing list of ids");
        skStringMapPrintMap(name_id_map, stdout);
        printf("\n");
    }

    printf("list should have "
           "{ \"baz\" }\n\t");
    skStringMapPrintMap(name_id_map, stdout);
    printf("\n\n");

    if (skStringMapRemoveByID(name_id_map, 3)
        != SKSTRINGMAP_OK)
    {
        printf("error removing entry by id");
        skStringMapPrintMap(name_id_map, stdout);
        printf("\n");
    }

    printf("list should have "
           "{ }\n\t");
    skStringMapPrintMap(name_id_map, stdout);
    printf("\n\n");

    err = skStringMapAddEntries(name_id_map,
                                (sizeof(a_few_more_ids)
                                 / sizeof(a_few_more_ids[0])),
                                a_few_more_ids);
    if (err != SKSTRINGMAP_OK) {
        printf("error %d (%s) adding list of a_few_more to\n\t",
               err, status_string(err));
        skStringMapPrintMap(name_id_map, stdout);
        printf("\n");
    }


    /* test normal usage cases */
    printf("Testing lookups in the map:\n\t");
    skStringMapPrintMap(name_id_map, stdout);
    printf("\n\n");


    for (i = 0; parseable[i].name != NULL; ++i) {
        test_string(name_id_map, parseable[i].name,
                    SKSTRINGMAP_OK, parseable[i].count);
    }

    /* test no match cases */
    for (i = 0; ambiguous[i] != NULL; ++i) {
        test_string(name_id_map, ambiguous[i], SKSTRINGMAP_PARSE_AMBIGUOUS, 0);
    }
    for (i = 0; no_match[i] != NULL; ++i) {
        test_string(name_id_map, no_match[i], SKSTRINGMAP_PARSE_NO_MATCH, 0);
    }

    /* test unparsable cases */
    for (i = 0; unparseable[i] != NULL; ++i) {
        test_string(name_id_map, unparseable[i],
                    SKSTRINGMAP_PARSE_UNPARSABLE, 0);
    }

    test_get_by_name(name_id_map);

    test_get_by_id(name_id_map);

    /* cleanup list */
    if (skStringMapDestroy(name_id_map) != SKSTRINGMAP_OK) {
        printf("error deallocating list\n");
    }

    return 0;
}


static void
test_string(
    sk_dllist_t            *name_id_map,
    const char             *user_input,
    sk_stringmap_status_t   desired_parse_status,
    size_t                  desired_entryc)
{
    sk_stringmap_iter_t *results = NULL;
    char *bad_token = NULL;
    sk_stringmap_status_t rv;
    sk_stringmap_entry_t *entry;

    printf("TEST MATCH BEGIN: %s\n", user_input);
    rv = skStringMapMatch(name_id_map, user_input, &results, &bad_token);
    if ((rv != desired_parse_status)
        || ((desired_parse_status == SKSTRINGMAP_OK)
            && (skStringMapIterCountMatches(results) != desired_entryc)))
    {
        printf("  TEST FAILED, parse status %d (%s) entryc %u",
               rv,
               status_string(rv),
               (unsigned int)skStringMapIterCountMatches(results));
        if (bad_token != NULL) {
            printf(" failed-token '%s'", bad_token);
            free(bad_token);
        }
        printf("\n  WHILE MATCHING\n\t%s\nIN LIST\n\t", user_input);
        skStringMapPrintMap(name_id_map, stdout);
        printf("\n");
    } else {
        printf("  SUCCESS %d (%s)",
               desired_parse_status, status_string(desired_parse_status));
        if (desired_parse_status == SKSTRINGMAP_OK) {
            printf("  [");
            while (skStringMapIterNext(results, &entry, NULL)==SK_ITERATOR_OK){
                printf(" %d", entry->id);
            }
            printf(" ]");
        } else if (bad_token != NULL) {
            printf(" failed-token '%s'", bad_token);
            free(bad_token);
        }
        printf("\n");
    }

    printf("TEST END\n\n");
    if (results) {
        skStringMapIterDestroy(results);
    }
}


static void
test_get_by_name(
    sk_stringmap_t     *name_id_map)
{
    sk_stringmap_entry_t *entry;
    sk_stringmap_status_t rv;
    const char *name[] = {"baz", "BAZ", "Baz", "bAz", "baZ", "BaZ", NULL};
    int i;

    for (i = 0; name[i] != NULL; ++i) {
        printf("TEST GET_BY_NAME BEGIN: %s\n", name[i]);
        rv = skStringMapGetByName(name_id_map, name[i], &entry);
        if (rv != SKSTRINGMAP_OK) {
            printf("  TEST FAILED, parse status %d (%s)",
                   rv,
                   status_string(rv));
            printf("\n  WHILE MATCHING\n\t%s\nIN LIST\n\t", name[i]);
            skStringMapPrintMap(name_id_map, stdout);
        } else {
            printf("  SUCCESS %d (%s)",
                   rv, status_string(rv));
            printf(" [%d]", entry->id);
        }
        printf("\n");
        printf("TEST END\n\n");
    }
}


static void
test_get_by_id(
    sk_stringmap_t     *name_id_map)
{
    sk_stringmap_iter_t *iter;
    sk_stringmap_entry_t add_entry;
    sk_stringmap_entry_t *map_entry;
    const char *name;

    printf("list of names should be moo and orc\n");

    memset(&add_entry, 0, sizeof(add_entry));
    add_entry.name = "moo";
    add_entry.id = 999;
    if (skStringMapAddEntries(name_id_map, 1, &add_entry) != SKSTRINGMAP_OK) {
        printf("error adding moo\n");
    }
    add_entry.name = "orc";
    if (skStringMapAddEntries(name_id_map, 1, &add_entry) != SKSTRINGMAP_OK) {
        printf("error adding orc\n");
    }

    name = skStringMapGetFirstName(name_id_map, (sk_stringmap_id_t)999);
    if (name == NULL || 0 != strcmp("moo", name)) {
        printf("CRITICAL FAILURE getting first name for id 999\n");
        return;
    }

    if (skStringMapGetByID(name_id_map, (sk_stringmap_id_t)999, &iter)
        != SKSTRINGMAP_OK)
    {
        printf("CRITICAL FAILURE looking up names for id 999\n");
        return;
    }

    while (skStringMapIterNext(iter, &map_entry, NULL) == SK_ITERATOR_OK) {
        printf("  found name %s\n", map_entry->name);
    }

    skStringMapIterDestroy(iter);
}


static const char *
status_string(
    sk_stringmap_status_t   st)
{
    switch (st) {
      case SKSTRINGMAP_OK:
        return "OK";
      case SKSTRINGMAP_PARSE_AMBIGUOUS:
        return "AMBIGUOUS";
      case SKSTRINGMAP_PARSE_NO_MATCH:
        return "NO_MATCH";
      case SKSTRINGMAP_PARSE_UNPARSABLE:
        return "UNPARSABLE";
      case SKSTRINGMAP_ERR_PARSER:
        return "PARSER_ERR";
      case SKSTRINGMAP_ERR_INPUT:
        return "INPUT";
      case SKSTRINGMAP_ERR_MEM:
        return "MEM";
      case SKSTRINGMAP_ERR_LIST:
        return "LIST";
      case SKSTRINGMAP_ERR_DUPLICATE_ENTRY:
        return "DUPLICATE_ENTRY";
      case SKSTRINGMAP_ERR_ZERO_LENGTH_ENTRY:
        return "ZERO_LENGTH_ENTRY";
      case SKSTRINGMAP_ERR_NUMERIC_START_ENTRY:
        return "NUMERIC_START_ENTRY";
      case SKSTRINGMAP_ERR_ALPHANUM_START_ENTRY:
        return "ALPHANUM_START_ENTRY";
      default:
        return "UNKNONW";
    }
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
