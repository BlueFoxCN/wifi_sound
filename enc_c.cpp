/*

This example reads from the default PCM device
and writes to standard output for 5 seconds of data.

*/

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include <speex/speex.h>



int main() {
	long loops;
	int rc;
	int size;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	unsigned int val;
	int dir;
	snd_pcm_uframes_t frames;
	char *buffer;
	int channel_num = 1;

	/* Open PCM device for recording (capture). */
	rc = snd_pcm_open(&handle, "default",
	 				SND_PCM_STREAM_CAPTURE, 0);
	if (rc < 0) {
		fprintf(stderr,
				"unable to open pcm device: %s\n",
				snd_strerror(rc));
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

	/* 44100 bits/second sampling rate (CD quality) */
	val = 44100;
	snd_pcm_hw_params_set_rate_near(handle, params,
									  &val, &dir);

	/* Set period size to 32 frames. */
	frames = 32;
	snd_pcm_hw_params_set_period_size_near(handle,
								  params, &frames, &dir);

	/* Write the parameters to the driver */
	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		fprintf(stderr,
				"unable to set hw parameters: %s\n",
				snd_strerror(rc));
		exit(1);
	}

	/* Use a buffer large enough to hold one period */
	snd_pcm_hw_params_get_period_size(params,
									&frames, &dir);
	size = frames * 2 * channel_num; /* 2 bytes/sample, 2 channels */
	buffer = (char *) malloc(size);
	

	/* We want to loop for 5 seconds */
	snd_pcm_hw_params_get_period_time(params,
									&val, &dir);
	loops = 5000000 / val;

/************************encode***********************************************/
	int frames_size;
	frames_size=channel_num*16;	/* 16 bits/sample, 2 channels */
	void *enc_state;
	SpeexBits enc_bits;
	char c_out[size];
	short out[size];
	int nbBytes;

	enc_state = speex_encoder_init(&speex_nb_mode);

	int q=8;
	speex_encoder_ctl(enc_state,SPEEX_SET_QUALITY,&q);  

	speex_encoder_ctl(enc_state,SPEEX_GET_FRAME_SIZE,&frames_size );

	speex_bits_init(&enc_bits);  


	while (loops > 0) 
	{	
		/*every period,read the numbers of frames bytes datas to buffer from PCM*/
		loops--;
		rc = snd_pcm_readi(handle, c_out, frames);
		if (rc == -EPIPE) 
		{
			/* EPIPE means overrun */
			fprintf(stderr, "overrun occurred\n");
			snd_pcm_prepare(handle);
		}
		 else if (rc < 0) 
		{
			fprintf(stderr,
				  "error from read: %s\n",
				  snd_strerror(rc));
		}
		 else if (rc != (int)frames)
		{
			fprintf(stderr, "short read, read %d frames\n", rc);
		}
		
		
		for(int i=0;i<size;i++)
			out[i]=c_out[i];
		

		speex_bits_reset(&enc_bits);
		speex_encode_int(enc_state,out,&enc_bits);
		nbBytes = speex_bits_write(&enc_bits, buffer, size);
		
		rc = write(1, buffer, nbBytes);
		if (rc != nbBytes)
			fprintf(stderr,
				  "short write: wrote %d bytes\n", rc);
	
		/*
		rc = write(1, buffer, size);
		if (rc != size)
			fprintf(stderr,
				  "short write: wrote %d bytes\n", rc);
		*/

		/*every period,encode*/		
	}

	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	free(buffer);

	speex_decoder_destroy(enc_state);
	speex_bits_destroy(&enc_bits);


	return 0;
}
