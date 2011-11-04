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

#ifndef DEFINES_H
#define DEFINES_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef ENDIAN_LITTLE
	#include <boost/detail/endian.hpp>
	#ifdef BOOST_LITTLE_ENDIAN
		#define ENDIAN_LITTLE
	#endif
#endif

#if defined(_WIN32) || defined(_WIN64)

#ifndef __WORDSIZE
	#if defined(_WIN32)
		#define __WORDSIZE 32
	#elif defined(_WIN64)
		#define __WORDSIZE 64
	#endif
#endif

#ifndef PRId32
#define PRId32 "I32d"
#endif

#ifndef PRId64
#define PRId64 "I64d"
#endif

#ifndef PRIu32
#define PRIu32 "I32u"
#endif

#ifndef PRIu64
#define PRIu64 "I64u"
#endif

#ifndef PRIx32
#define PRIx32 "I32x"
#endif

#ifndef PRIx64
#define PRIx64 "I64x"
#endif

#ifndef PRIX32
#define PRIX32 "I32X"
#endif

#ifndef PRIX64
#define PRIX64 "I64X"
#endif

#else

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

#define _snprintf snprintf
#define _strtoui64 strtoull
#define _strtoi64 strtoll

#endif // defined(_WIN32) || defined(_WIN64)

#include <boost/cstdint.hpp>
using namespace boost;

#include <string>
#include <list>
#include <vector>
#include <map>
#include <set>
using namespace std;

#endif // DEFINES_H
