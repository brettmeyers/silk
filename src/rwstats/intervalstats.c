/*
** Copyright (C) 2001-2014 by Carnegie Mellon University.
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
** intervalstats.c:
**
**      support library to calculate statistics from interval-based
**      frequency distributions.
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: intervalstats.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/utils.h>
#include "interval.h"


/* EXTERNAL VARIABLES */

/*
 * intervals are defined for each protocol separately. Till we decide
 * we want to change it, treat icmp like udp
 */
uint32_t tcpByteIntervals[NUM_INTERVALS] = {
    40,60,100,150,256,1000,10000,100000,1000000,0xFFFFFFFF
};
uint32_t udpByteIntervals[NUM_INTERVALS] = {
    20,40,80,130,256,1000,10000,100000,1000000,0xFFFFFFFF
};
uint32_t tcpPktIntervals[NUM_INTERVALS] = {
    3,4,10,20,50,100,500,1000,10000,0xFFFFFFFF
};
uint32_t udpPktIntervals[NUM_INTERVALS] = {
    3,4,10,20,50,100,500,1000,10000,0xFFFFFFFF
};
uint32_t tcpBppIntervals[NUM_INTERVALS] = {
    40,44,60,100,200,400,600,800,1500,0xFFFFFFFF
};
uint32_t udpBppIntervals[NUM_INTERVALS] = {
    20,24,40,100,200,400,600,800,1500,0xFFFFFFFF
};


/* LOCAL VARIABLES */

static uint32_t* cumFrequencies = NULL;
static double retArray[3];      /* return results via this buffer */
static uint64_t total;
static uint32_t quartileIndices[3];
static uint32_t quartileValues[3];

/* FUNCTION DECLARATIONS */

static double
getQuantile(
    const uint32_t     *data,
    const uint32_t     *boundaries,
    uint32_t            numIntervals,
    uint32_t            quantile);
static void
intervalSetup(
    const uint32_t     *data,
    uint32_t            numIntervals);
static void intervalTeardown(void);


/* FUNCTION DEFINITIONS */

static void
intervalTeardown(
    void)
{
    if (cumFrequencies != NULL) {
        free(cumFrequencies);
        cumFrequencies = NULL;
    }
}

/* pure noops */
int
intervalInit(
    void)
{
    return 0;
}

void
intervalShutdown(
    void)
{
    return;
}


/*
 * intervalSetup:
 *      Compute cum freq into cumFrequencies vector.
 *      Mark 25, 50, 75th percentile interval indices.
 *      Record cumulative total in global total.
 * Input:
 *      data vector
 * Output: None
 * Side Effects: globals cumFrequencies, quartileIndices, total set.
 */
static void
intervalSetup(
    const uint32_t     *data,
    uint32_t            numIntervals)
{
    register uint32_t i;

    if (cumFrequencies != NULL) {
        free(cumFrequencies);
    }

    cumFrequencies = (uint32_t *)malloc(numIntervals * sizeof(uint32_t));
    cumFrequencies[0] = data[0];
    for (i = 1; i < numIntervals; i++) {
        cumFrequencies[i] = cumFrequencies[i-1] + data[i];
    }
    total = cumFrequencies[numIntervals - 1];
    quartileValues[0] = total >> 2;             /* 1/4th */
    quartileValues[1] = total >> 1;
    quartileValues[2] = 3 * quartileValues[0];

    /* record quantile indices */
    for (i = 0; i < numIntervals; i++) {
        if (quartileValues[0] <= cumFrequencies[i]) {
            quartileIndices[0] = i;
            break;
        }
    }
    for (i = 0; i < numIntervals; i++) {
        if (quartileValues[1] <= cumFrequencies[i]) {
            quartileIndices[1] = i;
            break;
        }
    }
    for (i = 0; i < numIntervals; i++) {
        if (quartileValues[2] <= cumFrequencies[i]){
            quartileIndices[2] = i;
            break;
        }
    }
    return;
}

#define MC 0
#if     MC
/*
** getQuantile:
**      return the indicated quantile expressed as a number between 1-100
** Input: the desired quantile.
** Output: the quantile
** Side Effects: None
*/
static double
getQuantile(
    const uint32_t     *data,
    const uint32_t     *boundaries,
    uint32_t            numIntervals,
    uint32_t            quantile)
{
    int32_t intervalIndex;
    int32_t intervalOffset;
    double intervalSlope, result;

    /*
     * First, find the interval
     */
    if (quantile == 25) {
        intervalIndex = quartileIndices[0];
    } else if (quantile == 50) {
        intervalIndex = quartileIndices[1];
    } else if (quantile == 75) {
        intervalIndex = quartileIndices[2];
    } else {
        /* dont have it. Calculate the desired index */
        int i;
        uint32_t quantileValue = quantile * total;
        for (i = 0; i < numIntervals; i++) {
            if (quantileValue <= cumFrequencies[i]) {
                intervalIndex = i;
                break;
            }
        }
        if (i == numIntervals) {
            intervalIndex = numIntervals - 1;
        }
    }
    if (intervalIndex == 0) {
        /*
         * Calculate based on flat percentage difference
         */
        intervalSlope = (intervalIndex * 1.0)/(data[intervalIndex] * 1.0);
        return intervalSlope * data[intervalIndex];
    }

    if (intervalIndex == (numIntervals - 1) ) {
        /* in last interval which has a high value of 0xFFFFFFFF */
        return 0.00;
    }
    intervalOffset = cumFrequencies[intervalIndex]
        - cumFrequencies[intervalIndex -1];
    intervalSlope = (intervalOffset * 1.0) /
        (1.0 * data[intervalIndex]);
    result = (intervalSlope * (boundaries[intervalIndex]
                               - boundaries[intervalIndex - 1]))
        + (1.0 * boundaries[intervalIndex -1]);
    return result;
}
#else
static double
getQuantile(
    const uint32_t  UNUSED(*data),
    const uint32_t         *boundaries,
    uint32_t                numIntervals,
    uint32_t                quantile)
{
    /*
    ** Blo = boundary value at the lower index of the cumFreq containing
    **          the quantile value
    ** Bhi = boundary value at the higher index of the cumFreq containing
    **          the quantile value
    **
    ** Vq = cum freq at the quantile desired ( = quantile frac * total )
    **
    ** Vhi = cumfreq value >= Vq
    ** Vlo = cumfreq value 1 lower than Vhi
    */
    register uint32_t Blo, Bhi, Vlo, Vhi, Vq;
    register uint32_t intervalIndex = 0;
    register uint32_t i;

    /*
     * First, find the interval
     */
    if (quantile == 25) {
        intervalIndex = quartileIndices[0];
        Vq = quartileValues[0];
    } else if (quantile == 50) {
        intervalIndex = quartileIndices[1];
        Vq = quartileValues[1];
    } else if (quantile == 75) {
        intervalIndex = quartileIndices[2];
        Vq = quartileValues[2];
    } else {
        /* dont have it. Calculate the desired index */
        Vq = quantile * total;
        for (i = 0; i < numIntervals; i++) {
            if (Vq <= cumFrequencies[i]) {
                intervalIndex = i;
                break;
            }
        }
    }

    Bhi = boundaries[intervalIndex];
    Vhi = cumFrequencies[intervalIndex];
    if (intervalIndex == 0) {
        Blo = 0;
        Vlo = 0;
    } else {
        Blo = boundaries[intervalIndex - 1 ];
        Vlo = cumFrequencies[intervalIndex - 1 ];
    }
    return Blo + ( (double) (Vq -Vlo) / (double) (Vhi - Vlo) )
        * (double)(Bhi - Blo);
}
#endif


double *
intervalQuartiles(
    const uint32_t     *data,
    const uint32_t     *boundaries,
    uint32_t            numIntervals)
{
    intervalSetup(data, numIntervals);
    retArray[0] = getQuantile(data, boundaries, numIntervals, 25);
    retArray[1] = getQuantile(data, boundaries, numIntervals, 50);
    retArray[2] = getQuantile(data, boundaries, numIntervals, 75);
    intervalTeardown();
    return retArray;
}


/*
** intervalMoments:
**      calculate the mean and variance for interval freq data.
** Inputs: uint32_t vector of data, boundaries;
**         uint32_t # of elements in the vectors.
** Outputs: double *array of mean and var.
** Side Effects: None.
*/
double *
intervalMoments(
    const uint32_t         *data,
    const uint32_t  UNUSED(*boundaries),
    uint32_t                numIntervals)
{
    intervalSetup(data, numIntervals);

    intervalTeardown();
    return retArray;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
