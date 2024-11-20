#ifndef TIMER_H
#define TIMER_H

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[38;5;13m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_PINK  "\x1b[38;5;225m"
#define ANSI_COLOR_RESET   "\x1b[0m"

struct timer_id_t {
	int done;
	int fsh;
	pthread_cond_t event_cond;
	pthread_mutex_t event_lock;
	pthread_cond_t timer_cond;
	pthread_mutex_t timer_lock;
};

void start_timer();

void stop_timer();

struct timer_id_t * attach_event();

void detach_event(struct timer_id_t * event);

void next_slot(struct timer_id_t* timer_id);

uint64_t current_time();

#endif
