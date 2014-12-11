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
**  skstringmap.h
**
**    An ordered list of name->id mappings to support named fields,
**    where name is a C-string; id is a uint32_t.
**
*/
#ifndef _SKSTRINGMAP_H
#define _SKSTRINGMAP_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_STRINGMAP_H, "$SiLK: skstringmap.h cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/silk_types.h>

/**
 *  @file
 *
 *    An ordered list of name->id mappings to support named fields,
 *    where name is a C-string; id is a uint32_t.
 *
 *    This file is part of libsilk.
 *
 *
 *    Name matching in a string map is case-insensitive.
 *
 *    The basic usage is to create a sk_stringmap_t and to add
 *    sk_stringmap_entry_t items to it, each of which is a name/value
 *    pair.
 *
 *    Then, once processing begins, call skStringMapParse() with the
 *    user's string, and it will return either a valid result set (an
 *    sk_stringmap_iter_t), or a parse error.
 *
 *    Sample Usage (does not check error cases)
 *
 *      sk_stringmap_t *name_id_map;
 *      sk_stringmap_entry_t mappings[] = {
 *          { "foo", 1, "Description of foo", NULL },
 *          { "bar", 2, "Description of bar", NULL },
 *      };
 *      char *user_input = "foo,baz";
 *      sk_stringmap_iter_t *iter;
 *      sk_stringmap_status_t rv;
 *      sk_stringmap_id_t found_id;
 *      sk_stringmap_dupes_t dupes = SKSTRINGMAP_DUPES_KEEP;
 *
 *      // Create map and add entries
 *      skStringMapCreate( &name_id_map );
 *      skStringMapAddEntries( name_id_map, 2, mappings );
 *
 *      // Map user's input and process each entry
 *      rv = skStringMapParse( name_id_map, user_input, dupes, &iter, NULL );
 *      if( rv == SKSTRINGMAP_OK ) {
 *          sk_stringmap_entry_t *entry;
 *          while (skStringMapIterNext(iter, &entry, NULL) == SK_ITERATOR_OK) {
 *              found_id = entry->id;
 *              // DO STUFF WITH THE IDS MATCHED
 *          }
 *      }
 *
 *      // Clean up
 *      skStringMapIterDestroy( iter );
 *      skStringMapDestroy( name_id_map );
 */


/* typedef sk_dllist_t sk_stringmap_t;  // silk_types.h */

/**
 *    Type of the integer key for an entry in the string map.
 */
typedef uint32_t sk_stringmap_id_t;

/**
 *    Description of an entry in the string map.
 */
typedef struct sk_stringmap_entry_st {
    /** string name key */
    const char             *name;
    /** unsigned integer id value */
    sk_stringmap_id_t       id;
    /** optional description of this entry */
    const char             *description;
    /** data pointer maintained by the caller */
    const void             *userdata;
} sk_stringmap_entry_t;


/**
 *    Can be put at end of an array of sk_stringmap_entry_t to
 *    indicate no-more-entries.
 */
#define SK_STRINGMAP_SENTINEL   {NULL, 0, NULL, NULL}


/**
 *    Iterator over the results of parsing
 */
typedef struct sk_stringmap_iter_st sk_stringmap_iter_t;


/**
 *    Function result status
 */
typedef enum {
    /** Command was successful */
    SKSTRINGMAP_OK = 0,

    /** Indicates bad input, e.g. NULL pointer */
    SKSTRINGMAP_ERR_INPUT = -127,

    /** A memory allocation call failed */
    SKSTRINGMAP_ERR_MEM,

    /** Some unexpected error occured in the linked list */
    SKSTRINGMAP_ERR_LIST,

    /* The following values can be returned while adding a new
     * key/value pair to the map. */

    /** The new key was found to be a duplicate of a key already in the
     * map. */
    SKSTRINGMAP_ERR_DUPLICATE_ENTRY,

    /** The new key was found to be the empty string. */
    SKSTRINGMAP_ERR_ZERO_LENGTH_ENTRY,

    /** The key was found to start with a number but to contain
     * non-numeric characters. */
    SKSTRINGMAP_ERR_NUMERIC_START_ENTRY,

    /** The key was found to start with a non-alphanumeric
     * character. */
    SKSTRINGMAP_ERR_ALPHANUM_START_ENTRY,

    /** The parser encountered an unexpected error unrelated to the
     * user's input */
    SKSTRINGMAP_ERR_PARSER,

    /* The following values can be returned while parsing user input
     * and finding entries in the map. */

    /** The user's input is not an exact match nor a prefix match for
     * any key. */
    SKSTRINGMAP_PARSE_NO_MATCH,

    /** The user's input matches matches no key exactly and is a prefix
     * match for multiple keys. */
    SKSTRINGMAP_PARSE_AMBIGUOUS,

    /** The user's input is not parsable or contains an invalid range
     * e.g., 3-2 */
    SKSTRINGMAP_PARSE_UNPARSABLE,

    /** Returned when parsing is successful but the 'handle_dupes'
     * parameter was set to SKSTRINGMAP_DUPES_REMOVE_WARN.  This
     * indicates an warning message is in the error buffer. */
    SKSTRINGMAP_OK_DUPLICATE = 1

} sk_stringmap_status_t;


/**
 *    How parse functions should handle duplicate entries
 */
typedef enum {
    SKSTRINGMAP_DUPES_KEEP = 0,
    SKSTRINGMAP_DUPES_REMOVE_SILENT,
    SKSTRINGMAP_DUPES_REMOVE_WARN,
    SKSTRINGMAP_DUPES_ERROR
} sk_stringmap_dupes_t;



/**
 *    Create a new string-map and puts the result into 'out_str_map'.
 *    Return SKSTRINGMAP_OK on success, or SKSTRINGMAP_ERR_MEM if
 *    allocation fails.
 *
 */
sk_stringmap_status_t
skStringMapCreate(
    sk_stringmap_t    **out_str_map);


/**
 *    Destroy the string-map 'str_map'.
 *
 */
sk_stringmap_status_t
skStringMapDestroy(
    sk_stringmap_t     *str_map);


/**
 *    Return 1 if the string-map 'str_map' is empty; 0 if it contains
 *    entries.
 */
#define skStringMapIsEmpty(str_map) skDLListIsEmpty(str_map)


/**
 *    Add multiple entries from the 'entryv' array to the StringMap
 *    'str_map'.
 *
 *    When 'entryc' is positive or zero, it is taken as the count of
 *    entries in 'entryv'.  When 'entryc' is negative, 'entryv' is
 *    considered to be a NULL-terminated array of entries; i.e., all
 *    entries in 'entryv' are added one of the form
 *    SK_STRINGMAP_SENTINEL is reached.
 *
 *    Returns SKSTRINGMAP_OK if all entries are successfully added.
 *    If an error occurs, some entries may have been added to the
 *    StringMap.  Returns SKSTRINGMAP_ERR_INPUT if 'str_map' or
 *    'entryv' is NULL, or if 'entryc' is positive and a "name" in
 *    entryv is NULL.  Returns SKSTRINGMAP_ERR_MEM if allocation
 *    fails.  May also return any of SKSTRINGMAP_ZERO_LENGTH_ENTRY,
 *    SKSTRINGMAP_ERR_NUMERIC_START_ENTRY,
 *    SKSTRINGMAP_ERR_ALPHANUM_START_ENTRY,
 *    SKSTRINGMAP_DUPLICATE_ENTRY.
 */
sk_stringmap_status_t
skStringMapAddEntries(
    sk_stringmap_t             *str_map,
    int                         entryc,
    const sk_stringmap_entry_t *entryv);


/**
 *    Remove the single entry from the StringMap 'str_map' whose name
 *    is 'name'.  Return SKSTRINGMAP_ERR_INPUT if 'str_map' or 'name'
 *    is NULL, else return SKSTRINGMAP_OK.
 */
sk_stringmap_status_t
skStringMapRemoveByName(
    sk_stringmap_t     *str_map,
    const char         *name);


/**
 *    Remove all entries from the StringMap 'str_map' whose ID is
 *    'id'.  Return SKSTRINGMAP_ERR_INPUT if 'str_map' is NULL, else
 *    return SKSTRINGMAP_OK.
 */
sk_stringmap_status_t
skStringMapRemoveByID(
    sk_stringmap_t     *str_map,
    sk_stringmap_id_t   id);


/**
 *    Remove multiple entries from the StringMap 'str_map' by calling
 *    skStringMapRemoveByName() for all entries in 'entryv'.
 *
 *    When 'entryc' is positive or zero, it is taken as the count of
 *    entries in 'entryv'.  When 'entryc' is negative, 'entryv' is
 *    considered to be a NULL-terminated array of entries; i.e., all
 *    entries in 'entryv' are removed one of the form
 *    SK_STRINGMAP_SENTINEL is reached.
 */
sk_stringmap_status_t
skStringMapRemoveEntries(
    sk_stringmap_t             *str_map,
    int                         entryc,
    const sk_stringmap_entry_t *entryv);


/**
 *    Take the user entered string, 'user_string' containing tokens
 *    separated by commas(,) and hyphens(-), parse it to get a list of
 *    keys, search the StringMap 'str_map' for those keys, allocate a
 *    sk_stringmap_iter_t to iterate over the entries, and store the
 *    location of the iterator at the memory location point to by
 *    'iter'.
 *
 *    If the 'user_string' contains duplicate entries, the
 *    corresponding sk_stringmap_entry_t will be added to the iterator
 *    multiple times.
 *
 *    When non-NULL, 'bad_token' should contain a pointer to a
 *    C-string to hold the token that was being parsed when a parse
 *    error occurred.  It is the caller's responsibility to free()
 *    this string.
 *
 *    When the caller is finished with the iterator, she must call
 *    skStringMapIterDestroy() to free the resources used by the
 *    iterator.
 */
sk_stringmap_status_t
skStringMapMatch(
    const sk_stringmap_t   *str_map,
    const char             *user_string,
    sk_stringmap_iter_t   **iter,
    char                  **bad_token);


/**
 *    Take the user entered string, 'user_string' containing tokens
 *    separated by commas(,) and hyphens(-), parse it to get a list of
 *    keys, search the StringMap 'str_map' for those keys, allocate a
 *    sk_stringmap_iter_t to iterate over the entries, and store the
 *    location of the iterator at the memory location point to by
 *    'iter'.
 *
 *    The value 'handle_dupes' specifies how to handle duplicate
 *    entries in the 'user_string'.  The values are:
 *
 *       SKSTRINGMAP_DUPES_KEEP - duplicate the sk_stringmap_entry_t
 *       in the 'out_vec'
 *
 *       SKSTRINGMAP_DUPES_REMOVE_SILENT - completely ignore the
 *       duplicate entry
 *
 *       SKSTRINGMAP_DUPES_REMOVE_WARN - do not store the duplicate,
 *       but inform the user of the issue
 *
 *       SKSTRINGMAP_DUPES_ERROR - do not store the duplicate and
 *       return an error code
 *
 *    If an error occurs and 'errmsg' is non-NULL, it will be set to a
 *    static buffer containing an error message that the caller can
 *    print.  The static buffer remains valid until the next call to
 *    this function.
 *
 *    When the caller is finished with the iterator, she must call
 *    skStringMapIterDestroy() to free the resources used by the
 *    iterator.
 */
sk_stringmap_status_t
skStringMapParse(
    const sk_stringmap_t   *str_map,
    const char             *user_input,
    sk_stringmap_dupes_t    handle_dupes,
    sk_stringmap_iter_t   **iter,
    char                  **errmsg);


/**
 *    Parse the string 'user_string' in a manner similar to
 *    skStringMapParse(), but in addition store any attributes for the
 *    entries.
 *
 *    Having different attributes for the same token does not make the
 *    tokens different as far as the checking for duplicates is
 *    concerned.  For example, the user_string "foo:1,foo:2" will
 *    warn/error on the duplicate token "foo" when 'handle_dupes' is
 *    not set to SKSTRINGMAP_DUPES_KEEP.
 */
sk_stringmap_status_t
skStringMapParseWithAttributes(
    const sk_stringmap_t   *str_map,
    const char             *user_string,
    sk_stringmap_dupes_t    handle_dupes,
    sk_stringmap_iter_t   **iter,
    char                  **errmsg);


/**
 *    Take the user entered string 'user_string' containing a SINGLE
 *    key and search the StringMap 'str_map' to find the
 *    sk_stringmap_entry_t that has that string as a key, setting
 *    'out_entry' to point to that key and returning SKSTRINGMAP_OK.
 *    The caller should not modify nor free the returned entry.
 *
 *    Return SKSTRINGMAP_PARSE_AMBIGUOUS if 'user_string' matches
 *    multiple entries, SKSTRINGMAP_PARSE_NO_MATCH if it matches no
 *    entries.
 */
sk_stringmap_status_t
skStringMapGetByName(
    const sk_stringmap_t   *str_map,
    const char             *user_string,
    sk_stringmap_entry_t  **out_entry);


/**
 *    Find the entry matching 'user_string' in a manner similar to
 *    skStringMapGetByName(), but in addition copy any attributes into
 *    the 'attributes' buffer, a character array of size
 *    'attributes_len'.  If there are no attributes for the entry, set
 *    'attributes' to the empty string.
 *
 *    In addition to the return values that skStringMapGetByName()
 *    returns, return SKSTRINGMAP_PARSE_UNPARSABLE if syntax
 *    designating the attribute is incorrect or if the attribute will
 *    not fit into the 'attributes' buffer.
 */
sk_stringmap_status_t
skStringMapGetByNameWithAttributes(
    const sk_stringmap_t   *str_map,
    const char             *user_string,
    sk_stringmap_entry_t  **out_entry,
    char                   *attributes,
    size_t                  attributes_len);


/**
 *    Given a numeric identifier 'id', allocate a sk_stringmap_iter_t
 *    to iterate over the entries with that identifier, and store the
 *    location of the iterator at the memory location point to by
 *    'iter'.
 *
 *    When the caller is finished with the iterator, she must call
 *    skStringMapIterDestroy() to free the resources used by the
 *    iterator.
 */
sk_stringmap_status_t
skStringMapGetByID(
    const sk_stringmap_t   *str_map,
    sk_stringmap_id_t       id,
    sk_stringmap_iter_t   **iter);


/**
 *    Find the first entry in the StringMap 'str_map' whose ID is 'id'
 *    and return the name associated with the entry.  Return NULL if
 *    'id' is not in the StringMap.  The caller should not modify nor
 *    free the returned value.
 */
const char *
skStringMapGetFirstName(
    const sk_stringmap_t   *str_map,
    sk_stringmap_id_t       id);


/**
 *    Return the number of matches (entries) contained in the
 *    stringmap iterator 'iter'.
 */
size_t
skStringMapIterCountMatches(
    sk_stringmap_iter_t    *iter);


/**
 *    Destroy the iterator pointed at by 'iter'.
 */
void
skStringMapIterDestroy(
    sk_stringmap_iter_t    *iter);


/**
 *    If more entries are available in the stringmap iterator 'iter',
 *    set the memory at 'entry' to point to the next entry, set the
 *    memory at 'attr' to point at the next attribute string for that
 *    entry, and return SK_ITERATOR_OK; otherwise, return
 *    SK_ITERATOR_NO_MORE_ENTRIES.  If there is no attribute for the
 *    entry, the memory at 'attr' is set to point to an empty string.
 *
 *    If 'attr' is NULL, the attributes string is not returned.
 *
 *    The caller should not modify nor free the returned entry or
 *    attributes.
 */
int
skStringMapIterNext(
    sk_stringmap_iter_t    *iter,
    sk_stringmap_entry_t  **entry,
    const char            **attr);


/**
 *    Reset the stringmap iterator 'iter' so it may loop over the
 *    matched entries again.
 */
void
skStringMapIterReset(
    sk_stringmap_iter_t    *iter);



/**
 *    Print the map keys and values to the given file stream.  The map
 *    is printed in a human readable format of the form
 *
 *        { "key1" : value1, "key2" : value2, ... }
 *
 *  Arguments
 *
 *    FILE *outstream - the output stream to which to print
 *
 *    sk_stringmap_t *str_map - pointer to the StringMap to print
 */
sk_stringmap_status_t
skStringMapPrintMap(
    const sk_stringmap_t   *str_map,
    FILE                   *outstream);


/**
 *    Print to the file handle 'fh' the names of the fields in the
 *    string map 'str_map' as a block of text indented by 'indent_len'
 *    spaces.
 *
 *    The function assumes that names that the map to the same
 *    identifier occur consecutively in the map.
 */
void
skStringMapPrintUsage(
    const sk_stringmap_t   *str_map,
    FILE                   *fh,
    const int               indent_len);


/**
 *    Print to the file handle 'fh' the names and descriptions of the
 *    fields in the string map 'str_map'.
 *
 *    The function assumes that names that the map to the same
 *    identifier occur consecutively in the map.  The first name is
 *    taken as the primary name; additional names are taken as
 *    aliases.
 *
 *    Primary names occur in a single column.  The descriptions and
 *    aliases occur in a second column, with line wrapping as needed.
 *    All text is indented from the left margin by a tab character.
 *    Code assumes the display width is 80 characters with a 8-space
 *    tabs.
 */
void
skStringMapPrintDetailedUsage(
    const sk_stringmap_t   *str_map,
    FILE                   *fh);


/**
 *    Given the 'error_code' return a textual representation of it.
 */
const char *
skStringMapStrerror(
    int                 error_code);


#ifdef __cplusplus
}
#endif
#endif /* _SKSTRINGMAP_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
