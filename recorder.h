#ifndef RECORDER_H
#define RECORDER_H

#include <thread>
#include <string>
#include <unordered_map>
#include <list>
using namespace std;

// #define TEST_RELAY

#define RECORD_TIME_FILE "record_time.txt"

#define CHECK_INTERNAL 60

class Recorder {
  private:
    bool is_record;
    bool stop;
    void record();
  public:
    Recorder(string, list<string>);
    void check_record_time();
    thread start_record();
    void stop_record();
};

#endif
