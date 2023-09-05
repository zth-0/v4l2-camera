#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/v4l2-common.h>
#include <linux/v4l2-controls.h>

#include "debug.h"
#include "v4l2_camera_impl.h"

// An extended version of ioctl system function
#define xioctl(fd, request, data, ...)  {\
	int ok; \
	do {	\
		ok = ioctl(fd, request, data); \
	} while (ok < 0 && (errno == EINTR || errno == EAGAIN)); \
	check(ok >= 0, ##__VA_ARGS__); \
} \

static void _init_v4l2_buffer(struct v4l2_buffer *buf, int index) {
	if (!buf) {
		log_warn("attempted to init a NULL v4l2_buffer.");
		return;
	}
	
	buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf->memory = V4L2_MEMORY_MMAP;
	buf->index = index;
}

static void _deallocate_buffers(vcam_impl_t *cam) {
	if (!cam || !cam->frames) return;

	for (int i = 0; i < cam->n_frames; i++) {
		if (cam->frames[i].data) {
			munmap(cam->frames[i].data, cam->frames[i].length);
			cam->frames[i].data = NULL;
			cam->frames[i].length = 0;
			cam->frames[i].byteused = 0;
		}
	}

	free(cam->frames);
	cam->frames = NULL;
}

static void _close_device(int descriptor) {
	// Check if the file descriptor is still valid
	errno = 0;
	if (fcntl(descriptor, F_GETFL) != -1 || errno != EBADF) {
		close(descriptor);
	}
}

int vcam_impl_init(vcam_impl_t *vcam, const int *frame_num) {
	if (!vcam) return FAILURE;
	
	if (!frame_num) {
		vcam->n_frames = DEFAULT_FRAME_COUNT;
	} else {
		vcam->n_frames = *frame_num;
	}

	return SUCCESS;
}

void vcam_impl_destroy(vcam_impl_t  *cam) {
	if (!cam) return;

    if (cam->path) free(cam->path);
	_deallocate_buffers(cam);
	_close_device(cam->descriptor);
}

int vcam_impl_init_stream(vcam_impl_t *cam, struct v4l2_format *format) {
	check_mem(cam);

	cam->frames = (frame_buffer*)malloc(cam->n_frames * sizeof(frame_buffer));
	check_mem(cam->frames);

	debug("Open camera device file: %s", cam->path);
	cam->descriptor = open(cam->path, O_RDWR);
	check(cam->descriptor >= 0, "failed to open camera device %s", cam->path);

	debug("Query capability");
	struct v4l2_capability capability;
	xioctl(cam->descriptor, VIDIOC_QUERYCAP, &capability, "ioctl error: %s", "VIDIOC_QUERYCAP");

	debug("Set format");
	struct v4l2_format stream_fmt;
	struct v4l2_format *pfmt = format;

	if (!pfmt) {
		stream_fmt.fmt.pix.width = DEFAULT_PIX_W;
		stream_fmt.fmt.pix.height = DEFAULT_PIX_H;
		stream_fmt.fmt.pix.pixelformat = DEFAULT_FRAME_COUNT;
		pfmt = &stream_fmt;
	}

	xioctl(cam->descriptor, VIDIOC_S_FMT, pfmt, "ioctl error: %s", "VIDIOC_S_FMT");

	debug("Request buffers");
	struct v4l2_requestbuffers req_buf = {0};
	req_buf.type = stream_fmt.type;
	req_buf.count = cam->n_frames;
	req_buf.memory = V4L2_MEMORY_MMAP;
	xioctl(cam->descriptor, VIDIOC_REQBUFS, &req_buf, "ioctl error: %s", "VIDIOC_REQBUFS");

    debug("Query buffers");
	for (int i = 0; i < cam->n_frames; i++) {
		struct v4l2_buffer query_buf = {0};
		_init_v4l2_buffer(&query_buf, i);
		xioctl(cam->descriptor, VIDIOC_QUERYBUF, &query_buf, "ioctl error: %s", "VIDIOC_QUERYBUF");

		char *buf = (char*)mmap(NULL, query_buf.length,
								PROT_READ | PROT_WRITE,
								MAP_SHARED,
								cam->descriptor, query_buf.m.offset);

		check(buf != MAP_FAILED, "mmap failed.");
		cam->frames[i].data = buf;
		cam->frames[i].length = query_buf.length;
		cam->frames[i].byteused = 0;
	}
	
	return SUCCESS;
	
error:
	return FAILURE;
}

static int _stream_execute(vcam_impl_t *cam, unsigned long action) {
	check_mem(cam);

	int valid_action = action == VIDIOC_STREAMON || action == VIDIOC_STREAMOFF;
	check(valid_action, "Either VIDIOC_STREAMON or VIDIOC_STREAMOFF");

	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	xioctl(cam->descriptor, action, &type, "ioctl error: %s", "VIDIOC_STREAMON / VIDIOC_STREAMOFF");
	
	return SUCCESS;	

error:
	return FAILURE;
}

int vcam_impl_start_stream(vcam_impl_t *cam) {
	return _stream_execute(cam, VIDIOC_STREAMON);
}

int vcam_impl_stop_stream(vcam_impl_t *cam) {
	_stream_execute(cam, VIDIOC_STREAMOFF);
	_deallocate_buffers(cam);
	_close_device(cam->descriptor);
}

int vcam_impl_capture_frames(vcam_impl_t *cam) {
	check_mem(cam);
	for (int i = 0; i < cam->n_frames; i++) {
		struct v4l2_buffer q_buf = {0};
		_init_v4l2_buffer(&q_buf, i);
		xioctl(cam->descriptor, VIDIOC_QBUF, &q_buf, "ioctl error: %s", "VIDIOC_QBUF");
		xioctl(cam->descriptor, VIDIOC_DQBUF, &q_buf, "ioctl error: %s", "VIDIOC_DQBUF");
		cam->frames[q_buf.index].byteused = q_buf.bytesused;
	}
	
	return SUCCESS;

error:
	return FAILURE;
}
