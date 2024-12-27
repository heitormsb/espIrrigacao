#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include <sys/time.h>

void time_sync_notification_cb(struct timeval *tv);
void initialize_sntp(void);
void obtain_time(void);

#endif // TIME_SYNC_H
