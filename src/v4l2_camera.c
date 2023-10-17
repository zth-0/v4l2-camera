
#include <stdlib.h>

#include "debug.h"
#include "v4l2_camera.h"

static void _stop_stream(vcam_t *vcam) {
	pthread_mutex_lock(&vcam->_mux);
	vcam->stream_status = OFF;
	vcam->_out = NULL;
	vcam_impl_stop_stream(vcam->impl);
	pthread_mutex_unlock(&vcam->_mux);
}

static int _can_stream(vcam_t *vcam) {
	return vcam->stream_status == ON && vcam->_out != NULL;
}

// run start the streaming thread
static void* _run(void *input) {
	vcam_t *vcam = (vcam_t*)input;
	
	// wait for the stream_status to be 
	// set to ON in main thread.

	log_info("Streaming thread started...");
	pthread_mutex_lock(&vcam->_mux);
	while (!_can_stream(vcam)) {
		log_info("Waiting for signal to start streaming...");
		pthread_cond_wait(&vcam->_scv, &vcam->_mux);
	}
	pthread_mutex_unlock(&vcam->_mux);

	int rc = 0;
	log_info("Streaming started.");
	while (_can_stream(vcam)) {
		rc = vcam_impl_capture_frames(vcam->impl);
		if (!rc) {
			log_err("Failed to capture frames.");
			continue;
		}

		for (int i = 0; i < vcam->impl->n_frames; i++) {
			frame_buffer *fbuf = &vcam->impl->frames[i];
			rc = fwrite(fbuf, fbuf->byteused, 1, vcam->_out);
			if (rc == -1) {
				log_err("Failed to write frame to stream.");
			}
		}

		rc = fflush(vcam->_out);
		if (rc == -1) {
			log_err("Failed to flush frame to stream.");
		}
	}

	log_info("Streaming stopped.");
	return NULL;
} 

int vcam_init_stream(vcam_t *vcam, struct v4l2_format *fmt) {
	if (!vcam) return 0;

	pthread_mutex_lock(&vcam->_mux);
	int ok = 0;
	int valid_status = vcam->stream_status == OFF;
	if (valid_status) {
		log_info("Initiating streaming.");
		ok = vcam_impl_init_stream(vcam->impl, fmt);
	}	
	pthread_mutex_unlock(&vcam->_mux);

	if (ok) {
		pthread_create(&vcam->_sth, NULL, _run, vcam);
		return 1;
	}

	return 0;
}

int vcam_start_stream(vcam_t *vcam, FILE *out) {
	if (!vcam) return 0;

	int ok = 0;
	pthread_mutex_lock(&vcam->_mux);
	if (vcam->stream_status == OFF) {
		log_info("Starts streaming.");
		vcam->stream_status = ON;
		vcam->_out = out;

		ok = vcam_impl_start_stream(vcam->impl);
		if (ok) {
			pthread_cond_signal(&vcam->_scv);
		}
	}
	
	return ok;
}

void vcam_stop_stream(vcam_t *vcam) {
	if (!vcam) return;
	_stop_stream(vcam);
	pthread_join(vcam->_sth, NULL);
}

void vcam_destroy(vcam_t *vcam) {
	if (!vcam) return;
	_stop_stream(vcam);
	vcam_impl_destroy(vcam->impl);
	
	int ret;
	ret = pthread_cond_destroy(&vcam->_scv);
	if (!ret) log_warn("Failed to destroy mutex");

	ret = pthread_mutex_destroy(&vcam->_mux);
	if (!ret) log_warn("Failed to destroy conditional_variable");
}