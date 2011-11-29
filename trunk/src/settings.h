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

#ifndef SETTINGS__H
#define SETTINGS__H

#include "defines.h"
#include "xixibase.h"

class Settings {
public:
	Settings();
	void init();
	bool load_conf();
	bool ext_to_mime(const string& ext, string& mime_type);

	string home_dir;

	uint64_t maxbytes;
	uint32_t maxconns;
	uint16_t port;
	string inter;

	double factor;            // chunk size growth factor
	uint32_t pool_size;       // number of io_service to run
	uint32_t num_threads;     // number of threads to run
	uint32_t item_size_min;
	uint32_t item_size_max;

	uint32_t max_stats_group;

	map<string, string> mime_map_;
};

extern Settings settings_;

#endif // SETTINGS__H
