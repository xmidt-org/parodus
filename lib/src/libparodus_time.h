/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/
#ifndef  _LIBPARODUS_TIME_H
#define  _LIBPARODUS_TIME_H

#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>

#define TIMESTAMP_LEN 19
#define TIMESTAMP_BUFLEN (TIMESTAMP_LEN+1)

#define NONE_TIME "00000000-000000-000"

/*
 * 	timestamps are stored as 19 byte character strings:
 *  YYYYMMDD-HHMMSS-mmm
 *  where the last 3 bytes are  milliseconds
*/

/**
 * Get current time
 *
 * @param tv	current time
 * @param split_time  time broken down
 * @return 0 on success, valid errno otherwise.
 */
int get_current_time (struct timeval *tv, struct tm *split_time);

/**
 * Extract date from struct tm 
 *
 * @param split_time  time broken down
 * @param date 8 byte buffer where date is stored (YYYYMMDD)
 * @return None
 */
void extract_date (struct tm *split_time, char *date);

/**
 * Get current date
 *
 * @param date 8 byte buffer where date is stored (YYYYMMDD)
 * @return 0 on success, valid errno otherwise.
 */
int get_current_date (char *date);

/**
 * Create a timestamp 
 *
 * @param split_time  time broken down
 * @param msecs milliseconds
 * @param timestamp 20 byte buffer (19 byte time + ending null)
 */
void make_timestamp (struct tm *split_time, unsigned msecs, char *timestamp);

/**
 * Create a timestamp for the current time
 *
 * @param timestamp 20 byte buffer (19 byte time + ending null)
 * @return 0 on success, valid errno otherwise.
 */
int make_current_timestamp (char *timestamp);


/**
 * Get absolute expiration timespec, given delay in ms
 *
 * @param timestamp 20 byte buffer (19 byte time + ending null)
 * @param ms  delay in msecs
 * @param ts  expiration time
 * @return 0 on success, valid errno otherwise.
 */
int get_expire_time (uint32_t ms, struct timespec *ts);

#endif

