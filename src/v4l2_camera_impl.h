#ifndef __V4L2_CAMERA_IMPL_h__
#define __V4L2_CAMERA_IMPL_h__

#include <linux/videodev2.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_PIX_W 		1024 // default width of a frame
#define DEFAULT_PIX_H 		1024 // default height of a frame
#define DEFAULT_FRAME_COUNT 128

#define SUCCESS 1
#define FAILURE 0

typedef struct frame_buffer {
	char 	*data;
	size_t 	length;
	size_t 	byteused;
} frame_buffer;

typedef struct v4l2_camera_impl {	
	char*			path;
	int 			descriptor;
	int 			n_frames;
	frame_buffer*	frames;
} vcam_impl_t;

vcam_impl_t* vcam_impl_create(const char* dev_path, const int* frame_num);

void vcam_impl_destroy(vcam_impl_t	*cam);

int vcam_impl_init_stream(vcam_impl_t *cam, struct v4l2_format *format);

int vcam_impl_start_stream(vcam_impl_t *cam);

void vcam_impl_stop_stream(vcam_impl_t *cam);

// vcam_impl_capture_frames load the captured frames into the frames// of the v4l2_camera_impl. 
// This function needs to be called // every time before copying frame data from frames.
int vcam_impl_capture_frames(vcam_impl_t *cam);

#ifdef __cplusplus
}
#endif

#endif
