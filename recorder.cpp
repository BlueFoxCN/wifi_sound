#include <iostream>
#include <cstring>
#include <thread>
#include <list>
#include <unordered_map>
#include <alsa/asoundlib.h>
#include <time.h>
#include "recorder.h"
#include "timer.h"
#include "tool.h"
#include "log.h"

using namespace std;

Recorder::Recorder() {
  this->is_record = false;
  this->stop = false;
}

thread Recorder::start_record() {
  // start the record thread
  thread record_thread(&Recorder::record, this);
  return record_thread;
}

void Recorder::stop_record() {
  stop = true;
}

void Recorder::record() {
  log_trace("start record thread");
  // the audio initialization part
  long loops;
  int rc;
  int size;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  unsigned int val;
  int dir;
  snd_pcm_uframes_t frames;
  int channel_num = 1;

  /* Open PCM device for recording (capture). */
  rc = snd_pcm_open(&handle, "hw:0,0",
          SND_PCM_STREAM_CAPTURE, 0);
  if (rc < 0) {
    log_error("unable to open pcm device: %s", snd_strerror(rc));
    exit(1);
  }

  /* Allocate a hardware parameters object. */
  snd_pcm_hw_params_alloca(&params);

  /* Fill it in with default values. */
  snd_pcm_hw_params_any(handle, params);

  /* Set the desired hardware parameters. */

  /* Interleaved mode */
  snd_pcm_hw_params_set_access(handle, params,
                SND_PCM_ACCESS_RW_INTERLEAVED);

  /* Signed 16-bit little-endian format */
  snd_pcm_hw_params_set_format(handle, params,
                SND_PCM_FORMAT_S16_LE);

  /* Two channels (stereo) */
  snd_pcm_hw_params_set_channels(handle, params, channel_num);

  /* 48000 bits/second sampling rate (CD quality) */
  do {
    val = 48000;
    snd_pcm_hw_params_set_rate_near(handle, params,
                  &val, &dir);
  } while (val != 48000);

  /* Set period size to 48 frames. */
  frames = 48;
  snd_pcm_hw_params_set_period_size_near(handle,
                    params, &frames, &dir);

  /* Write the parameters to the driver */
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    log_error("unable to set hw parameters: %s", snd_strerror(rc));
    exit(1);
  }

  /* Use a buffer large enough to hold one period */
  snd_pcm_hw_params_get_period_size(params,
                  &frames, &dir);
  size = frames * 2 * channel_num; /* 2 bytes/sample, 2 channels */

  snd_pcm_hw_params_get_period_time(params,
                  &val, &dir);


  // the network part
  int fd = socket(AF_INET,SOCK_DGRAM,0);
  if(fd==-1) {
    log_error("socket create error!");
    exit(-1);
  }

  struct sockaddr_in addr_to;//目标服务器地址
  addr_to.sin_family = AF_INET;
  addr_to.sin_port = htons(RECV_DATA_PORT);
  addr_to.sin_addr.s_addr = inet_addr(this->parent.c_str());

  struct sockaddr_in addr_from;
  addr_from.sin_family = AF_INET;
  addr_from.sin_port = htons(0);//获得任意空闲端口
  addr_from.sin_addr.s_addr = htons(INADDR_ANY);//获得本机地址
  int yes = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  int r = ::bind(fd, (struct sockaddr*)&addr_from, sizeof(addr_from));

  if (r == -1) {
    log_error("Bind error!");
    close(fd);
    exit(-1);
  }

  char buf[size];
  int len;
  char* data_with_ip;
  // send data to the server
  is_record = true;
  stop = false;
  while (!stop) {
    
    data_with_ip = new char[strlen(this->ip_addr.c_str()) + 1 + size];
    strcpy(data_with_ip, this->ip_addr.c_str());
    strcat(data_with_ip, ":");

    rc = snd_pcm_readi(handle, &data_with_ip[strlen(this->ip_addr.c_str()) + 1], frames);
    if (rc == -EPIPE) {
      // EPIPE means overrun
      log_warn("overrun occurred");
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      log_error("error from read: %s", snd_strerror(rc));
    } else if (rc != (int)frames) {
      log_warn("short read, read %d frames", rc);
    }

    len = sendto(fd, data_with_ip, strlen(this->ip_addr.c_str()) + 1 + size, 0, (struct sockaddr*)&addr_to, sizeof(addr_to)); 
    delete(data_with_ip);
  }

  is_record = false;

  close(fd);
  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  return;
}

void Recorder::check_record_time() {
  time_t cur_time;
  struct tm *p;
  int cur_second;
  File *fp;
  ssize_t read;
  char *line;
  char *pch;
  bool hit;
  int start_time, end_time;
  thread record_thread;
  while(true) {
    hit = false;
    time(&cur_time);
    p = gmtime(&cur_time);
    cur_second = p->tm_hour * 3600 + p->tm_min * 60 + p->tm_sec;
    fp = fopen(RECORD_FILE_NAME, "r");
    if (fp) {
      while ((read = getline(&line, 0, fp)) != -1) {
        start_time = atoi(strtok(line, ","));
        end_time = atoi(strtok(line, ","));
        if (cur_time > start_time && cur_time < end_time) {
          hit = true;
          break;
        }
      }
      fclose(file);
    }
    if (is_record && !hit) {
      stop_record();
      if (record_thread != NULL) {
        record_thread.join();
      }
    } else if (!is_record && hit) {
      record_thread = start_record();
    }
  }
}
