/*
** Copyright (C) 2001-2015 by Carnegie Mellon University.
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
**  sku-times.c
**
**  various utility functions for dealing with time
**
**  Suresh L Konda
**  1/24/2002
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: sku-times.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/utils.h>


/* Time to ASCII */
char *
sktimestamp_r(
    char               *outbuf,
    sktime_t            t,
    int                 timestamp_flags)
{
    struct tm ts;
    struct tm *rv;
    imaxdiv_t t_div;
    time_t t_sec;
    const int form_mask = (SKTIMESTAMP_NOMSEC | SKTIMESTAMP_EPOCH
                           | SKTIMESTAMP_MMDDYYYY | SKTIMESTAMP_ISO);

    t_div = imaxdiv(t, 1000);
    t_sec = (time_t)(t_div.quot);

    if (timestamp_flags & SKTIMESTAMP_EPOCH) {
        if (timestamp_flags & SKTIMESTAMP_NOMSEC) {
            snprintf(outbuf, SKTIMESTAMP_STRLEN-1, ("%" PRIdMAX), t_div.quot);
        } else {
            snprintf(outbuf, SKTIMESTAMP_STRLEN-1, ("%" PRIdMAX ".%03" PRIdMAX),
                     t_div.quot, t_div.rem);
        }
        return outbuf;
    }

    switch (timestamp_flags & (SKTIMESTAMP_UTC | SKTIMESTAMP_LOCAL)) {
      case SKTIMESTAMP_UTC:
        /* force UTC */
        rv = gmtime_r(&t_sec, &ts);
        break;
      case SKTIMESTAMP_LOCAL:
        /* force localtime */
        rv = localtime_r(&t_sec, &ts);
        break;
      default:
        /* use default timezone */
#if  SK_ENABLE_LOCALTIME
        rv = localtime_r(&t_sec, &ts);
#else
        rv = gmtime_r(&t_sec, &ts);
#endif
        break;
    }
    if (NULL == rv) {
        memset(&ts, 0, sizeof(struct tm));
    }

    switch (timestamp_flags & form_mask) {
      case SKTIMESTAMP_EPOCH:
      case (SKTIMESTAMP_EPOCH | SKTIMESTAMP_NOMSEC):
        /* these should have been handled above */
        skAbortBadCase(timestamp_flags & form_mask);
        break;

      case SKTIMESTAMP_MMDDYYYY:
        /* "MM/DD/YYYY HH:MM:SS.sss" */
        snprintf(outbuf, SKTIMESTAMP_STRLEN-1,
                 ("%02d/%02d/%04d %02d:%02d:%02d.%03" PRIdMAX),
                 ts.tm_mon + 1, ts.tm_mday, ts.tm_year + 1900,
                 ts.tm_hour, ts.tm_min, ts.tm_sec, t_div.rem);
        break;

      case (SKTIMESTAMP_MMDDYYYY | SKTIMESTAMP_NOMSEC):
        /* "MM/DD/YYYY HH:MM:SS" */
        snprintf(outbuf, SKTIMESTAMP_STRLEN-1, "%02d/%02d/%04d %02d:%02d:%02d",
                 ts.tm_mon + 1, ts.tm_mday, ts.tm_year + 1900,
                 ts.tm_hour, ts.tm_min, ts.tm_sec);
        break;

      case SKTIMESTAMP_ISO:
        /* "YYYY-MM-DD HH:MM:SS.sss" */
        snprintf(outbuf, SKTIMESTAMP_STRLEN-1,
                 ("%04d-%02d-%02d %02d:%02d:%02d.%03" PRIdMAX),
                 ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday,
                 ts.tm_hour, ts.tm_min, ts.tm_sec, t_div.rem);
        break;

      case (SKTIMESTAMP_ISO | SKTIMESTAMP_NOMSEC):
        /* "YYYY-MM-DD HH:MM:SS" */
        snprintf(outbuf, SKTIMESTAMP_STRLEN-1, "%04d-%02d-%02d %02d:%02d:%02d",
                 ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday,
                 ts.tm_hour, ts.tm_min, ts.tm_sec);
        break;

      case SKTIMESTAMP_NOMSEC:
        /* "YYYY/MM/DDTHH:MM:SS" */
        snprintf(outbuf, SKTIMESTAMP_STRLEN-1, "%04d/%02d/%02dT%02d:%02d:%02d",
                 ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday,
                 ts.tm_hour, ts.tm_min, ts.tm_sec);
        break;

      default:
        /* "YYYY/MM/DDTHH:MM:SS.sss" */
        snprintf(outbuf, SKTIMESTAMP_STRLEN-1,
                 ("%04d/%02d/%02dT%02d:%02d:%02d.%03" PRIdMAX),
                 ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday,
                 ts.tm_hour, ts.tm_min, ts.tm_sec, t_div.rem);
        break;
    }

    return outbuf;
}


char *
sktimestamp(
    sktime_t            t,
    int                 timestamp_flags)
{
    static char t_buf[SKTIMESTAMP_STRLEN];
    return sktimestamp_r(t_buf, t, timestamp_flags);
}



/*
 *  max_day = skGetMaxDayInMonth(year, month);
 *
 *    Return the maximum number of days in 'month' in the specified
 *    'year'.
 *
 *    NOTE:  Months are in the 1..12 range and NOT 0..11
 *
 */
int
skGetMaxDayInMonth(
    int                 yr,
    int                 mo)
{
    static int month_days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

    /* use a 0-based array */
    assert(1 <= mo && mo <= 12);

    /* if not February or not a potential leap year, return fixed
     * value from array */
    if ((mo != 2) || ((yr % 4) != 0)) {
        return month_days[mo-1];
    }
    /* else year is divisible by 4, need more tests */

    /* year not divisible by 100 is a leap year, and year divisible by
     * 400 is a leap year */
    if (((yr % 100) != 0) || ((yr % 400) == 0)) {
        return 1 + month_days[mo-1];
    }
    /* else year is divisible by 100 but not by 400; not a leap year */

    return month_days[mo-1];
}


/* like gettimeofday returning an sktime_t */
sktime_t
sktimeNow(
    void)
{
    struct timeval tv;

    (void)gettimeofday(&tv, NULL);
    return sktimeCreateFromTimeval(&tv);
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
