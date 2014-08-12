#include <iostream>
#include <thread>
#include <cstring>
#include "recorder.h"
#include "port.h"
#include "log.h"

using namespace std;

int main() {
  log_init(LL_DEBUG, "log", ".");
  Recorder recorder;
  Port port;

  thread t_recorder(&Recorder::check_record_time, recorder);
  thread t_port(&Port::start, port);

  t_recorder.join();
  t_port.join();
}
