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
#include <stdio.h>

//#define ENABLE_LINE_ID

#ifdef USING_BOOST_LOG
#include <boost/log/core.hpp>
#include <boost/log/filters.hpp>
#include <boost/log/formatters.hpp>
#include <boost/log/utility/init/to_file.hpp>
#include <boost/log/utility/init/to_console.hpp>
#include "boost/format.hpp"
#include <boost/thread/thread.hpp>
#include <boost/log/sources/logger.hpp>

namespace logging = boost::log;
namespace fmt = boost::log::formatters;
namespace flt = boost::log::filters;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace keywords = boost::log::keywords;

void log_init(const char* file_name, int rotation_size) {
	if (file_name == NULL || rotation_size <= 0) {
		return;
	}
	namespace logging = boost::log;
	namespace sinks = boost::log::sinks;
	namespace flt = boost::log::filters;
	namespace attrs = boost::log::attributes;
	namespace keywords = boost::log::keywords;
	namespace fmt = boost::log::formatters;
/*
	logging::init_log_to_file(
		// file name pattern
		keywords::file_name = file_name,
		// rotate files every x size
		keywords::rotation_size = rotation_size,
		// ... or at midnight
		keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
#ifdef ENABLE_LINE_ID
		keywords::format = fmt::format("%1% [%2% %3% %4%] %5%")
			% fmt::attr< unsigned int >("LineID")
#else
		keywords::format = fmt::format("[%1% %2% %3%] %4%")
#endif
			% fmt::date_time< boost::posix_time::ptime >("TimeStamp", keywords::format = "%Y-%m-%d %H:%M:%S.%f")
			% fmt::attr< boost::thread::id >("ThreadID")
			% fmt::attr< boost::log::trivial::severity_level >("Severity", std::nothrow)
			% fmt::message(),
		keywords::auto_flush = true);
*/
	logging::init_log_to_console(
		std::cout,
#ifdef ENABLE_LINE_ID
		keywords::format = fmt::format("%1% [%2% %3% %4%] %5%")
			% fmt::attr< unsigned int >("LineID")
#else
		keywords::format = fmt::format("[%1% %2% %3%] %4%")
#endif
			% fmt::date_time< boost::posix_time::ptime >("TimeStamp", keywords::format = "%Y-%m-%d %H:%M:%S.%f")
			% fmt::attr< boost::thread::id >("ThreadID")
			% fmt::attr< boost::log::trivial::severity_level >("Severity", std::nothrow)
			% fmt::message(),
		keywords::auto_flush = true);
}
#else
boost::mutex log_lock_;

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>

const char* LOG_PREFIX(const char* severity) {
	static char log_prefix[100];

	std::string strTime = boost::posix_time::to_iso_string(boost::posix_time::microsec_clock::local_time());

	// 20111029T233826.031250
	if (strTime.size() < 22) {
		return "";
	}
/*
	std::string tmp = strTime.substr(0, 4)
		+ "-"
		+ strTime.substr(4, 2)
		+ "-"
		+ strTime.substr(6, 2)
		+ " "
		+ strTime.substr(9, 2)
		+ ":"
		+ strTime.substr(11, 2)
		+ ":"
		+ strTime.substr(13, 2)
		+ "."
		+ strTime.substr(16, 6);
	_snprintf(log_prefix, sizeof(log_prefix), "[%s %08"PRIx32" %s] ", tmp.c_str(), (uint32_t)boost::this_thread::get_id(), severity);
*/
	const char* p = strTime.c_str();
	_snprintf(log_prefix, sizeof(log_prefix), "[%4.4s-%2.2s-%2.2s %2.2s:%2.2s:%s %08"PRIx32" %s] ",
		p, p + 4, p + 6, p + 9, p + 11, p + 13, (uint32_t)&boost::this_thread::get_id(), severity);

	return log_prefix;
}
#endif

int log_level_;

void set_log_level(int log_level) {
	log_level_ = log_level;
}
