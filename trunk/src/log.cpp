/*
   Copyright [2011] [Yao Yuan(yeaya@163.com)]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "log.h"

#ifdef USING_BOOST_LOG
#include <boost/log/core.hpp>
#include <boost/log/filters.hpp>
#include <boost/log/formatters.hpp>
#include <boost/log/utility/init/to_file.hpp>
#include <boost/log/utility/init/to_console.hpp>
#endif

int log_level_;

#ifndef USING_BOOST_LOG
boost::mutex log_lock_;
#endif

void log_init(const char* file_name, int rotation_size) {
#ifdef USING_BOOST_LOG
	if (file_name == NULL || rotation_size <= 0) {
		return;
	}
	namespace logging = boost::log;
	namespace sinks = boost::log::sinks;
	namespace flt = boost::log::filters;
	namespace attrs = boost::log::attributes;
	namespace keywords = boost::log::keywords;

	logging::init_log_to_file(
		// file name pattern
		keywords::file_name = file_name,
		// rotate files every x size
		keywords::rotation_size = rotation_size,
		// ... or at midnight
		keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
		// log record format
		keywords::format = "[%TimeStamp% %ThreadID%]: %_%",
		keywords::auto_flush = true);

	logging::init_log_to_console(
		std::cerr,
		// log record format
		keywords::format = "[%TimeStamp% %ThreadID%]: %_%",
		keywords::auto_flush = true);
#endif
}

void set_log_level(int log_level) {
	log_level_ = log_level;
}
