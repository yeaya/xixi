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

#ifndef HASH_H
#define HASH_H

#include "defines.h"

#ifdef ENDIAN_LITTLE
uint32_t hashlittle(const void* key, size_t length, uint32_t initval);
#define hash32(key, length, initval) hashlittle(key, length, initval)
#else
uint32_t hashbig(const void* key, size_t length, uint32_t initval);
#define hash32(key, length, initval) hashbig(key, length, initval)
#endif

#endif // HASH_H
