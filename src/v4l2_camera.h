#ifndef __V4L2_CAMERA_H__
#define __V4L2_CAMERA_H__

#include <fcntl.h>
#include <linux/videodev2.h>
#include <pthread.h>
#include <stdio.h>

#include "v4l2_camera_impl.h"

typedef enum STREAM_STATUS {
	OFF,
	ON
} STREAM_STATUS;

typedef struct v4l2_camera {

	vcam_impl_t *impl;
	STREAM_STATUS stream_status;

	pthread_mutex_t _mux;
	pthread_cond_t	_scv;
	pthread_t _sth;

    FILE* _out;
} vcam_t;

int vcam_init_stream(vcam_t *vcam, struct v4l2_format *fmt);

int vcam_start_stream(vcam_t *vcam, FILE *out);

void vcam_stop_stream(vcam_t *vcam);

void vcam_destroy(vcam_t *vcam);

#endif