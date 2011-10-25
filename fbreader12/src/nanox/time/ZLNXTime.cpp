/*
 * Copyright (C) 2008 Alexander Egorov <lunohod@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <stdio.h>
#include <time.h>
#include "ZLNXTime.h"

void taskFunction(int signo, siginfo_t* evp, void* ucontext)
{
	if(evp->si_value.sival_ptr) {
		((ZLRunnable*)evp->si_value.sival_ptr)->run();
	}
}

void ZLNXTimeManager::addTask(shared_ptr<ZLRunnable> task, int interval) {
	removeTask(task);
	if ((interval > 0) && !task.isNull()) {
		struct sigaction sigv;
		struct sigevent sigx;
		struct itimerspec val;
		timer_t t_id;

		sigemptyset(&sigv.sa_mask);
		sigv.sa_flags = SA_SIGINFO;
		sigv.sa_sigaction = taskFunction;

		if(sigaction(SIGUSR1, &sigv, 0) == -1) {
			perror("sigaction");
			return;
		}

		sigx.sigev_notify = SIGEV_SIGNAL;
		sigx.sigev_signo = SIGUSR1;
		sigx.sigev_value.sival_ptr = (void *)&*task;

		if(timer_create(CLOCK_REALTIME, &sigx, &t_id) == -1) {
			perror("timer_create");
			return;
		}

		val.it_value.tv_sec = 0;
		val.it_value.tv_nsec = interval * 1000;
		val.it_interval.tv_sec = 0;
		val.it_interval.tv_nsec = interval * 1000;

		if(timer_settime(t_id, 0, &val, 0) == -1) {
			perror("timer_settime");
			return;
		}

		myHandlers[task] = t_id;
	}
}

void ZLNXTimeManager::removeTask(shared_ptr<ZLRunnable> task) {
	std::map<shared_ptr<ZLRunnable>,timer_t>::iterator it = myHandlers.find(task);
	if (it != myHandlers.end()) {
		timer_delete(it->second);
		myHandlers.erase(it);
	}
}
