#ifndef WATCHDOG_SERVICE_H
#define WATCHDOG_SERVICE_H

void watchdog_service_init(void);
void watchdog_service_kick(void);
void watchdog_service_force_test_reset(void);

#endif
