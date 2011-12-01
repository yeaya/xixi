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

#include "settings.h"
#include "util.h"
#include "tinyxml.h"
#include <boost/filesystem.hpp>

Settings settings_;

Settings::Settings() {
	init();
}

void Settings::init() {
	boost::system::error_code ec;
	boost::filesystem::path scp = boost::filesystem::system_complete(boost::filesystem::path("../"), ec);
	if (ec) {
		scp = boost::filesystem::system_complete(boost::filesystem::current_path(ec), ec);
	}
	if (!ec) {
		home_dir = scp.string();
	}

	port = 7788;
	inter = "0.0.0.0";
	maxbytes = 768 * 1024 * 1024;
	maxconns = 1024;
	factor = 1.25;
	pool_size = 2;
	num_threads = 4;
	item_size_min = 48;
	item_size_max = 5 * 1024 * 1024;

	max_stats_group = 1024;

	cache_expiration = 600;
}

string Settings::load_conf() {
	string xmlfile = settings_.home_dir + "conf/web.xml";
	TiXmlDocument doc(xmlfile.c_str());
	if (!doc.LoadFile()) {
		return "[web.xml] load error";
	}

	TiXmlHandle hDoc(&doc);
	TiXmlHandle hRoot(0);

	TiXmlElement* first = hDoc.FirstChildElement().Element();
	if (first == NULL) {
		return "[web.xml] format error";
	}
	string name = first->Value();

	hRoot = TiXmlHandle(first);

	TiXmlElement* ele = hRoot.FirstChildElement("default-cache-expiration").Element();
	if (ele != NULL) {
		string t = ele->GetText();
		if (!safe_toui32(t.c_str(), t.size(), cache_expiration)) {
			return "[web.xml] reading default-cache-expiration error";
		}
	}

	TiXmlElement* mime = hRoot.FirstChild("mime-mapping").Element();
	while (mime != NULL) {
		const char* ext = NULL;
		const char* type = NULL;
//		TiXmlNode* node_ext = mime->FirstChild("extension");
		ele = mime->FirstChildElement("extension");
		if (ele != NULL) {
			ext = ele->GetText();
		}
	//	TiXmlNode* node_type = mime->FirstChild("mime-type");
		ele = mime->FirstChildElement("mime-type");
		if (ele != NULL) {
			type = ele->GetText();
		}
		if (ext != NULL && type != NULL) {
			mime_map[string(ext)] = string(type);
		}
		mime = mime->NextSiblingElement();
	}

	TiXmlElement* welcome = hRoot.FirstChild("welcome-file-list").FirstChildElement("welcome-file").Element();
	while (welcome != NULL) {
		const char* file = welcome->GetText();
		if (file != NULL) {
			welcome_file_list.push_back(string(file));
		}
		welcome = welcome->NextSiblingElement();
	}

	return "";
}

bool Settings::ext_to_mime(const string& ext, string& mime_type) {
	map<string, string>::iterator it = mime_map.find(ext);
	if (it != mime_map.end()) {
		mime_type = it->second;
		return true;
	}
	return false;
}
