#ifndef PORT_H
#define PORT_H

#include <thread>
#include <string>
#include "common.h"
using namespace std;

#define RECORD_TIME_FILE "record_time.txt"
#define RECORD_TIME_MGT_PORT 8001

class Port {
  private:
    thread record_time_io();
  public:
    Port();
    void start();
};

#endif
