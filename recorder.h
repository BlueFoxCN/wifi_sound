#ifndef RECORDER_H
#define RECORDER_H

#include <thread>
#include <string>
#include "common.h"
using namespace std;

#define RECORD_TIME_FILE "record_time.txt"

#define FILE_SIZE 1000000
#define CHECK_INTERVAL 3

class Recorder {
  private:
    bool is_record;
    bool stop;
    void record();
    thread start_record();
  public:
    Recorder();
    void check_record_time();
};

#endif
