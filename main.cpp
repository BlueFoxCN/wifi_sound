#include <iostream>
#include <thread>
#include <list>
#include <cstring>
#include "recorder.h"
#include "tool.h"
#include "log.h"

using namespace std;

int main() {
  log_init(LL_DEBUG, "log", "/root/");
  Recorder recorder();

  thread t_recorder(&Recorder::check_record_time, recorder);

  t_cmd.join();
  t_data.join();
}
