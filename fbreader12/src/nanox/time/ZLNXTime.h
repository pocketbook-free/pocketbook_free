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

#ifndef __ZLNXTIME_H__
#define __ZLNXTIME_H__

#include <map>

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
//#include <error.h>
#include <assert.h>

#include "../../unix/time/ZLUnixTime.h"

class ZLNXTimeManager : public ZLUnixTimeManager {

public:
	static void createInstance() { ourInstance = new ZLNXTimeManager(); }

	void addTask(shared_ptr<ZLRunnable> task, int interval);
	void removeTask(shared_ptr<ZLRunnable> task);
	void removeTaskInternal(shared_ptr<ZLRunnable> task) {assert(0);};

private:
	std::map<shared_ptr<ZLRunnable>,timer_t> myHandlers;
};

#endif /* __ZLNXTIME_H__ */
