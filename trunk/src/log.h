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

#ifndef LOG_H
#define LOG_H

#define ENABLE_LOG
//#define USING_BOOST_LOG

#define log_level_trace    0
#define log_level_debug    1
#define log_level_info     2
#define log_level_warning  3
#define log_level_error    4
#define log_level_fatal    5
#define log_level_no_log   6

extern int log_level_;

#ifndef ENABLE_LOG
	#ifdef USING_BOOST_LOG
		#define ENABLE_LOG
	#else
		#define LOG_LEVEL log_level_no_log
	#endif
#endif

#ifndef LOG_LEVEL
	#if defined(_DEBUG) 
		#define LOG_LEVEL log_level_trace
	#else
		#define LOG_LEVEL log_level_info
	#endif
#endif

void set_log_level(int log_level);

#ifdef USING_BOOST_LOG
	void log_init(const char* file_name, int rotation_size);

	#include <boost/log/trivial.hpp>

	#if LOG_LEVEL <= log_level_trace
		#define LOG_TRACE(x)  if (log_level_ <= log_level_trace)  BOOST_LOG_TRIVIAL(trace) << x
	#else
		#define LOG_TRACE(x)
	#endif

	#if LOG_LEVEL <= log_level_debug
		#define LOG_DEBUG(x)  if (log_level_ <= log_level_debug)  BOOST_LOG_TRIVIAL(debug) << x
	#else
		#define LOG_DEBUG(x)
	#endif

	#if LOG_LEVEL <= log_level_info
		#define LOG_INFO(x)    if (log_level_ <= log_level_info)  BOOST_LOG_TRIVIAL(info) << x
	#else
		#define LOG_INFO(x)
	#endif

	#if LOG_LEVEL <= log_level_warning
		#define LOG_WARNING(x)  if (log_level_ <= log_level_warning)BOOST_LOG_TRIVIAL(warning) << x
	#else
		#define LOG_WARNING(x)
	#endif

	#if LOG_LEVEL <= log_level_error
		#define LOG_ERROR(x)  if (log_level_ <= log_level_error)  BOOST_LOG_TRIVIAL(error) << x
	#else
		#define LOG_ERROR(x)
	#endif

	#if LOG_LEVEL <= log_level_fatal
		#define LOG_FATAL(x)  if (log_level_ <= log_level_fatal)  BOOST_LOG_TRIVIAL(fatal) << x
	#else
		#define LOG_FATAL(x)
	#endif

#else
	#define log_init(file_name, rotation_size)
	#include <iostream>
	#include <boost/thread/mutex.hpp>
	extern boost::mutex log_lock_;

	extern char* LOG_PREFIX(const char* severity);

	#if LOG_LEVEL <= log_level_trace
		#define LOG_TRACE(x)  if (log_level_ <= log_level_trace) { log_lock_.lock(); std::cout << LOG_PREFIX("trace") << x << endl; log_lock_.unlock(); }
	#else
		#define LOG_TRACE(x)
	#endif

	#if (LOG_LEVEL <= log_level_debug)
		#define LOG_DEBUG(x)  if (log_level_ <= log_level_debug) { log_lock_.lock(); std::cout << LOG_PREFIX("debug") << x << endl; log_lock_.unlock(); }
	#else
		#define LOG_DEBUG(x)
	#endif

	#if LOG_LEVEL <= log_level_info
		#define LOG_INFO(x)    if (log_level_ <= log_level_info) { log_lock_.lock(); std::cout << LOG_PREFIX("info") << x << endl; log_lock_.unlock(); }
	#else
		#define LOG_INFO(x)
	#endif

	#if LOG_LEVEL <= log_level_warning
		#define LOG_WARNING(x)  if (log_level_ <= log_level_warning) { log_lock_.lock(); std::cout << LOG_PREFIX("warning") << x << endl; log_lock_.unlock(); }
	#else
		#define LOG_WARNING(x)
	#endif

	#if LOG_LEVEL <= log_level_error
		#define LOG_ERROR(x)  if (log_level_ <= log_level_error) { log_lock_.lock(); std::cout << LOG_PREFIX("error") << x << endl; log_lock_.unlock(); }
	#else
		#define LOG_ERROR(x)
	#endif

	#if LOG_LEVEL <= log_level_fatal
		#define LOG_FATAL(x)  if (log_level_ <= log_level_fatal) { log_lock_.lock(); std::cout << LOG_PREFIX("fatal") << x << endl; log_lock_.unlock(); }
	#else
		#define LOG_FATAL(x)
	#endif
#endif

#endif // LOG_H
