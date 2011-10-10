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

#include "server.h"
#include <string>
#include "settings.h"
#include "log.h"
#include <iostream>
#include <boost/thread/tss.hpp>
#include <boost/shared_ptr.hpp>

#if defined(_WIN32)
void console_ctrl_function(DWORD ctrl_type) {
  printf("console_ctrl_function %d\n", ctrl_type);
  if (svr_ != NULL) {
    svr_->stop();
  }
}

BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
  switch (ctrl_type) {
  case CTRL_C_EVENT:
  case CTRL_BREAK_EVENT:
  case CTRL_CLOSE_EVENT:
  case CTRL_SHUTDOWN_EVENT:
    console_ctrl_function(ctrl_type);
    return TRUE;
  default:
    return FALSE;
  }
}
#else
#include <signal.h>
void sigproc(int sig)
{
  printf("sigproc %d\n", sig);
  if (svr_ != NULL) {
    svr_->stop();
  }
}

void sigpipeproc(int sig)
{
  // Do nothing
}
#endif // _WIN32

void print_usage() {
  std::cout << "\n"
    "   -p <num>      TCP port number to listen on (default: 7788)\n"
    "   -l <ip_addr>  interface to listen on (default: INADDR_ANY, all addresses)\n"
    "   -d            run as a daemon\n"
    "   -m <num>      max memory to use for items in megabytes (default: 768 MB)\n"
    "   -M            return error on memory exhausted (rather than removing items)\n"
    "   -loglevel     0 trace\n"
    "                 1 debug\n"
    "                 2 info\n"
    "                 3 warning\n"
    "                 4 error\n"
    "                 5 fatal\n"
    "                 6 no_log\n"
    "   -h            print help\n"
    "   -i            print license\n"
    "   -f <factor>   chunk size growth factor (default: 1.25)\n"
    "   -c <num>      number of core (default: 2)\n"
    "   -t <num>      number of threads to use (default: 4)\n"
    "   -n <bytes>    Min item size (default: 48)\n"
    "   -I <k bytes>  Max item size (default: 5MB, min: 1k, max: 128MB)\n"
    "   " VERSION ", Copyright [2011] [Yao Yuan(yeaya@163.com)]\n";
    return;
}

void print_license() {
  std::cout <<
    "   Copyright [2011] [Yao Yuan(yeaya@163.com)]\n"
    "\n"
    "   Licensed under the Apache License, Version 2.0 (the \"License\");\n"
    "   you may not use this file except in compliance with the License.\n"
    "   You may obtain a copy of the License at\n"
    "\n"
    "       http://www.apache.org/licenses/LICENSE-2.0\n"
    "\n"
    "   Unless required by applicable law or agreed to in writing, software\n"
    "   distributed under the License is distributed on an \"AS IS\" BASIS,\n"
    "   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
    "   See the License for the specific language governing permissions and\n"
    "   limitations under the License.";
}

#define cmdcmp(x, const_str) strncmp(x, const_str, sizeof(const_str))

int main(int argc, char* argv[]) {
  log_init("xixibase_%N.log", 20 * 1024 * 1024);
  set_log_level(log_level_info);

  for (int i = 1; i < argc; i++) {
    if (cmdcmp(argv[i], "-p") == 0 && i + 1 < argc) {
      i++;
      settings_.port = atoi(argv[i]);
    } else if (cmdcmp(argv[i], "-m") == 0 && i + 1 < argc) {
      i++;
      settings_.maxbytes = ((uint64_t)atoi(argv[i])) * 1024 * 1024;
    } else if (cmdcmp(argv[i], "-c") == 0 && i + 1 < argc) {
      i++;
      settings_.maxconns = atoi(argv[i]);
    } else if (cmdcmp(argv[i], "-h") == 0) {
      print_usage();
      exit(EXIT_SUCCESS);
    } else if (cmdcmp(argv[i], "-i") == 0) {
      print_license();
            exit(EXIT_SUCCESS);
    } else if (cmdcmp(argv[i], "-l") == 0 && i + 1 < argc) {
      i++;
      settings_.inter = argv[i];
    } else if (cmdcmp(argv[i], "-d") == 0) {
  //     do_daemonize = true;
    } else if (cmdcmp(argv[i], "-f") == 0 && i + 1 < argc) {
      i++;
      settings_.factor = atof(argv[i]);
      if (settings_.factor <= 1.0) {
        settings_.factor = 1.25;
        fprintf(stderr, "factor must be greater than 1.0, using 1.25\n");
      }
    } else if (cmdcmp(argv[i], "-n") == 0 && i + 1 < argc) {
      i++;
      int size = atoi(argv[i]);
      if (size <= 0) {
        fprintf(stderr, "Item min size must be greater than 0\n");
        exit(EXIT_FAILURE);
      } else {
        settings_.item_size_min = size;
      }
    } else if (cmdcmp(argv[i], "-c") == 0 && i + 1 < argc) {
      i++;
      int num = atoi(argv[i]);
      if (num > 0) {
        settings_.pool_size = num;
      }
    } else if (cmdcmp(argv[i], "-t") == 0 && i + 1 < argc) {
      i++;
      int num = atoi(argv[i]);
      if (num > 0) {
        settings_.num_threads = num;
      }
    } else if (cmdcmp(argv[i], "-loglevel") == 0 && i + 1 < argc) {
      i++;
      int level = atoi(argv[i]);
      if (level >= log_level_trace && level <= log_level_no_log) {
        set_log_level(level);
      } else {
        std::cerr << "Invalid log level: " << argv[i] << "\n";
      }
    } else if (cmdcmp(argv[i], "-I") == 0 && i + 1 < argc) {
      i++;
      int size_max = atoi(argv[i]) * 1024;
      if (size_max <= 0) {
        fprintf(stderr, "Item max size cannot be less than 1024 bytes.\n");
        exit(EXIT_FAILURE);
      }
      if (size_max > 1024 * 1024 * 128) {
        fprintf(stderr, "Cannot set item size limit higher than 128 MB.\n");
        exit(EXIT_FAILURE);
      }
      settings_.item_size_max = size_max;
    } else {
      fprintf(stderr, "Illegal argument \"%s\"\n", argv[i]);
      print_usage();
      exit(EXIT_FAILURE);
        }
    }

  LOG_INFO("xixibase start.");

  try {
#if defined(_WIN32)
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#else
    signal(SIGINT,  sigproc);
    signal(SIGHUP,  sigproc);
    signal(SIGQUIT,  sigproc);
    signal(SIGTERM,  sigproc);
    signal(SIGPIPE,  sigpipeproc);
#endif

    svr_ = new Server(settings_.pool_size, settings_.num_threads);

    svr_->start();

    svr_->run();

    LOG_INFO("destory instance");
    delete svr_;
    svr_ = NULL;
  } catch (std::exception& e) {
    LOG_FATAL("Exception: " << e.what());
  }
  LOG_INFO("xixibase stop.");

  return 0;
}