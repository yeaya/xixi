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

#include <ctype.h>
#include <errno.h>
#include "util.h"

#define I32_MAX    0x7fffffffL
#define UI32_MAX   0xffffffffUL
#define I64_MAX    (((int64_t)0x7fffffff << 32) | 0xffffffff)
#define I64_MIN    (-MSVCRT_I64_MAX-1)
#define UI64_MAX   (((uint64_t)0xffffffff << 32) | 0xffffffff)

bool safe_toui64(const uint8_t* data, uint32_t data_len, uint64_t& out) {
  uint64_t ret = 0;
  uint64_t temp = 0;

    if (data == NULL || data_len == 0) {
        return false;
    }

  while (data_len > 0 && isspace(*data)) {
    data_len--;
    data++;
  }

    if (*data == '-') {
    return false;
  } else if(*data == '+') {
        data++;
    data_len--;
  }

    while (data_len > 0) {
        char cur = tolower(*data);
        int v;

        if (isdigit(cur)) {
            v = *data - '0';
        } else {
      break;
        }

        data++;
    data_len--;

    temp = ret * 10 + v;
    if (temp < ret) {
      return false;
    }
        ret = temp;
    }

    out = ret;
  return true;
}

extern bool safe_toi64(const uint8_t* data, uint32_t data_len, int64_t& out) {
  bool negative = false;
  int64_t ret = 0;
  int64_t temp = 0;

    if (data == NULL || data_len == 0) {
        return false;
    }

  while (data_len > 0 && isspace(*data)) {
    data_len--;
    data++;
  }

  if (*data == '-') {
    negative = true;
    data++;
    data_len--;
  } else if(*data == '+') {
    data++;
    data_len--;
  }

  while (data_len > 0) {
    char cur = tolower(*data);
    int v;

    if (isdigit(cur)) {
      v = *data - '0';
    } else {
      break;
    }

    data++;
    data_len--;

    temp = ret * 10 + v;
    if (temp < ret) {
      return false;
    }
      ret = temp;
    }

    out = negative ? -ret : ret;
  return true;
}

extern bool safe_toui32(const uint8_t* data, uint32_t data_len, uint32_t& out) {
  uint32_t ret = 0;
  uint32_t temp = 0;

  if (data == NULL || data_len == 0) {
    return false;
  }

  while (data_len > 0 && isspace(*data)) {
    data_len--;
    data++;
  }

  if (*data == '-') {
    return false;
  } else if(*data == '+') {
    data++;
    data_len--;
  }

  while (data_len > 0) {
    char cur = tolower(*data);
    int v;

    if (isdigit(cur)) {
      v = *data - '0';
    } else {
      break;
    }

    data++;
    data_len--;

    temp = ret * 10 + v;
    if (temp < ret) {
      return false;
    }
      ret = temp;
    }

  out = ret;
  return true;
}

extern bool safe_toi32(const uint8_t* data, uint32_t data_len, int32_t& out) {
  bool negative = false;
  int32_t ret = 0;
  int32_t temp = 0;

  if (data == NULL || data_len == 0) {
    return false;
  }

  while (data_len > 0 && isspace(*data)) {
    data_len--;
    data++;
  }

  if (*data == '-') {
    negative = true;
    data++;
    data_len--;
  } else if(*data == '+') {
    data++;
    data_len--;
  }

  while (data_len > 0) {
    char cur = tolower(*data);
    int v;

    if (isdigit(cur)) {
      v = *data - '0';
    } else {
      break;
    }

    data++;
    data_len--;

    temp = ret * 10 + v;
    if (temp < ret) {
      return false;
    }
    ret = temp;
  }

  out = negative ? -ret : ret;
  return true;
}

bool safe_strtoui64(const char* str, uint64_t& out) {
  errno = 0;
  char* endptr = NULL;
  out = _strtoui64(str, &endptr, 10);
  if (errno == ERANGE) {
    return false;
  }
  if (isspace(*endptr) || (*endptr == '\0' && endptr != str)) {
    if ((int64_t)out < 0 && strchr(str, '-') != NULL) {
      return false;
    }
    return true;
  }
  return false;
}

bool safe_strtoi64(const char* str, int64_t& out) {
  errno = 0;
  char* endptr = NULL;
  out = _strtoi64(str, &endptr, 10);
  if (errno == ERANGE) {
    return false;
  }
  return (isspace(*endptr) || (*endptr == '\0' && endptr != str));
}

bool safe_strtoui32(const char* str, uint32_t& out) {
  errno = 0;
  char* endptr = NULL;
  out = strtoul(str, &endptr, 10);
  if (errno == ERANGE) {
    return false;
  }

  if (isspace(*endptr) || (*endptr == '\0' && endptr != str)) {
    if ((int32_t)out < 0) {
      if (strchr(str, '-') != NULL) {
        return false;
      }
    }
    return true;
  }

  return false;
}

bool safe_strtoi32(const char* str, int32_t& out) {
  errno = 0;
  char* endptr = NULL;
  out = strtol(str, &endptr, 10);
  if (errno == ERANGE) {
    return false;
  }
  return (isspace(*endptr) || (*endptr == '\0' && endptr != str));
}
