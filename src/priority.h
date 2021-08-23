/* SPDX-License-Identifier: BSD-2-CLAUSE */

#ifndef PRIORITY_H
#define PRIORITY_H

void set_affinity();
void set_affinity2(int cpu);
void pthread_prio(int scheduler, pthread_t thread, unsigned int nice);
void set_prio(int scheduler, unsigned int nice);

#endif
