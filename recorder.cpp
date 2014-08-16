#include <iostream>
#include <cstring>
#include <thread>
#include <alsa/asoundlib.h>
#include <speex/speex.h>
#include <time.h>
#include "recorder.h"
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
  int cur_file_size;
  FILE *fp;
  char *buffer, cur_file_name[100], file_name_with_path[100], final_name[100], final_name_with_path[100];

  /* Open PCM device for recording (capture). */
  rc = snd_pcm_open(&handle, "hw:1,0",
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
  val = 48000;
  snd_pcm_hw_params_set_rate_near(handle, params,
                    &val, &dir);

  /* Set period size to 160 frames. */
  frames = 160;
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
  buffer = (char *) malloc(size);

  snd_pcm_hw_params_get_period_time(params,
                  &val, &dir);


  /***** encode *****/
  int frames_size;
	frames_size = channel_num * 16;	/* 16 bits/sample, 1 channels */
	void *enc_state;
	SpeexBits enc_bits;
	char c_out[size];
	short out[size / 2];
	int nBytes;
	long cur_time;

	enc_state = speex_encoder_init(&speex_nb_mode);
	int q=8;
	speex_encoder_ctl(enc_state, SPEEX_SET_QUALITY, &q);  
	speex_encoder_ctl(enc_state, SPEEX_GET_FRAME_SIZE, &frames_size);
	speex_bits_init(&enc_bits);  
  /******************/


  is_record = true;
  stop = false;
  cur_time = get_sys_time();
  sprintf(cur_file_name, "%lu", cur_time);
  strcpy(file_name_with_path, FILE_PATH);
  strcat(file_name_with_path, cur_file_name);
  fp = fopen(file_name_with_path, "w+");
  cur_file_size = 0;
  while (!stop) {
    
    rc = snd_pcm_readi(handle, c_out, frames);
    if (rc == -EPIPE) {
      // EPIPE means overrun
      log_warn("overrun occurred");
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      log_error("error from read: %s", snd_strerror(rc));
    } else if (rc != (int)frames) {
      log_warn("short read, read %d frames", rc);
    }

    for (int i = 0; i < size / 2; i++) {
      out[i] = ( c_out[2 * i + 1] << 8 ) | (unsigned char)c_out[2 * i];
    }

    speex_bits_reset(&enc_bits);
    speex_encode_int(enc_state,out,&enc_bits);
    nBytes = speex_bits_write(&enc_bits, buffer, size);


    fwrite(&nBytes, sizeof(int), 1, fp);
    fwrite(buffer, sizeof(char), nBytes, fp);
    cur_file_size += nBytes;
    if (cur_file_size > FILE_SIZE) {
      fclose(fp);
      strcpy(final_name, "f_");
      strcat(final_name, cur_file_name);
      strcpy(final_name_with_path, FILE_PATH);
      strcat(final_name_with_path, final_name);
      rename(file_name_with_path, final_name_with_path);
      log_trace("%s", final_name_with_path);
      cur_time = get_sys_time();
      sprintf(cur_file_name, "%lu", cur_time);
      strcpy(file_name_with_path, FILE_PATH);
      strcat(file_name_with_path, cur_file_name);
      fp = fopen(file_name_with_path, "w+");
      cur_file_size = 0;
    }
  }

  snd_pcm_drain(handle);
  snd_pcm_close(handle);

  fclose(fp);
  strcpy(final_name, "f_");
  strcat(final_name, cur_file_name);
  strcpy(final_name_with_path, FILE_PATH);
  strcat(final_name_with_path, final_name);
  rename(file_name_with_path, final_name_with_path);

  is_record = false;

  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  free(buffer);

	speex_decoder_destroy(enc_state);
	speex_bits_destroy(&enc_bits);

  return;
}

void Recorder::check_record_time() {
  time_t cur_time;
  struct tm *p;
  FILE *fp;
  ssize_t read;
  char *content, *pch, buf[1000];
  bool hit;
  int cur_second, start_time, end_time, flag;
  thread record_thread;
  while(true) {
    hit = false;
    flag = 0;
    time(&cur_time);
    p = gmtime(&cur_time);
    cur_second = p->tm_hour * 3600 + p->tm_min * 60 + p->tm_sec;
    log_trace("*** current second is %d ***", cur_second);
    fp = fopen(RECORD_TIME_FILE, "r");
    if (fp) {
      read = fread(buf, sizeof(char), sizeof(buf), fp);
      content = strtok(buf, ";");
      pch = strtok(content, ",");
      while (pch != NULL) {
        if (flag == 0) {
          start_time = atoi(pch);
        } else {
          end_time = atoi(pch);
          if (cur_second > start_time && cur_second < end_time) {
            hit = true;
            break;
          }
        }
        flag = 1 - flag;
        pch = strtok(NULL, ",");
      }
      fclose(fp);
    }
    if (is_record && !hit) {
      stop = true;
      record_thread.join();
    } else if (!is_record && hit) {
      record_thread = start_record();
    }
    sleep(CHECK_INTERVAL);
  }
}
